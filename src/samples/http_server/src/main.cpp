#include <fstream>
#include <filesystem>
#include <tobasa/datetime.h>
#include <tobasa/json.h>
#include <tobasa/path.h>
#include <tobasa/format.h>
#include <tobasa/logger.h>
#include <tobasa/file_reader.h>
#include <tobasa/string_reader.h>
#include <tobasa/util.h>
#include <tobasa/path.h>
#include <tobasahttp/server/http_server.h>
#include <tobasahttp/mimetypes.h>

#include <tobasa/span.h>
#include "server_lib.h"

using namespace tbs;

static constexpr char kMinimalPageCss[] = R"css(
   body{margin:0;min-height:100vh;padding:24px 16px;box-sizing:border-box;font-family:Segoe UI,Arial,sans-serif;background:linear-gradient(180deg,#f8fbff 0%,#f3f6fb 100%);color:#1f2a37;display:flex;align-items:center;justify-content:center;}
   .card{width:min(560px,100%);box-sizing:border-box;background:#fff;border:1px solid #d9e2f1;border-radius:18px;padding:40px 32px;box-shadow:0 12px 30px rgba(15,23,42,.08);text-align:center;}
   h1{margin:0 0 12px;font-size:2rem;}
   p{margin:0;font-size:1rem;color:#5f6f86;line-height:1.6;}
   .badge{display:inline-flex;align-items:center;margin-bottom:16px;padding:7px 10px;border-radius:999px;background:#edf2ff;color:#2563eb;font-size:.82rem;font-weight:700;}
   )css";


class WebSocketCtxWrapper 
{
public:
   WebSocketCtxWrapper() 
   {
      createWebSocketContext();
   }

   ~WebSocketCtxWrapper()
   {
      std::cout << "~WebSocketCtxWrapper \n";
   }

   std::shared_ptr<http::WebSocketContext> wsContext;

   void createWebSocketContext();
};

static std::unique_ptr<WebSocketCtxWrapper> wsCtxWrapper = nullptr;


void WebSocketCtxWrapper::createWebSocketContext()
{
   wsContext = std::make_shared<http::WebSocketContext>();

   wsContext->onOpen = [this](http::WebSocketPtr conn)
   {
      std::cout << tbsfmt::format("[websocket:{}] connection started", conn->id());
      conn->identifier( util::getRandomString(6) );

      std::ostringstream out;
      out <<  "Welcome to Tobasa Web Socket Service" << std::endl;
      out <<  "Your connection ID: " << std::to_string(conn->id()) << std::endl;
      out <<  "Your User ID: "       << "user_" << std::to_string(conn->id()) << std::endl;
      out <<  ""  << std::endl;
      
      out <<  "To send message:" << std::endl;
      out <<  "syntax:  MESSAGE|{destination}|{data}" << std::endl;
      out <<  "         {destination}:  client id, or ALL for all clients"  << std::endl;
      out <<  "example: MESSAGE|"+ std::to_string(conn->id()) +"|send to my self " << std::endl;
      out <<  ""  << std::endl;

      conn->sendText(out.str());
   };

   wsContext->onClose = [](http::WebSocketPtr conn, int closeCode, const std::string& reason)
   {
      std::cout << tbsfmt::format("[websocket:{}] connection closed, code: {}, reason: {}", conn->id(), closeCode, reason) << std::endl;
   };

   wsContext->onPing = [](http::WebSocketPtr conn)
   {
      std::cout << tbsfmt::format("[websocket:{}] received PING", conn->id()) << std::endl;
   };

   wsContext->onPong = [](http::WebSocketPtr conn)
   {
      std::cout << tbsfmt::format("[websocket:{}] received PONG", conn->id()) << std::endl;
   };

   wsContext->onMessage = [this] (http::WebSocketPtr conn, const std::string& message)
   {
      std::cout << tbsfmt::format("[websocket:{}] received data: {}", conn->id(), message) << std::endl;

      // -------------------------------------------------------
      // syntax: MESSAGE|{destination}|{data}
      //        {destination}:  client id, or ALL for all clients

      if ( util::startsWith(message, "MESSAGE|" ) )
      {
         auto items = util::split(message,"|");
         if (items.size() == 3)
         {
            auto destination = items[1];
            if (util::isNumber(destination))
            {
               auto destinationId = std::stoll(destination);
               wsContext->sendText( tbsfmt::format("MESSAGE from ID {} : {}", conn->id(), items[2]), destinationId );
            }
            else
            {
               if (destination == "ALL")
                  wsContext->sendText( tbsfmt::format("MESSAGE from ID {} : {}", conn->id(), items[2]) );
               else
                  conn->sendText("Invalid MESSAGE destination syntax");
            }
         }
         else
            conn->sendText("Invalid MESSAGE syntax");
      }
      else
         conn->sendText("[echo] " + message);

   };

   wsContext->onError = [this](http::WebSocketPtr conn, const http::ErrorData& error)
   {
      std::cout << tbsfmt::format("[websocket:{}] Error code: {}, {}", conn->id(), error.code, error.message) << std::endl;
   }; 
}


/// Create simple status response html page
http::RequestStatus statusResult(const http::HttpContext& context, http::StatusCode statusCode)
{
   http::HttpStatus status(statusCode);

   std::string content = "<html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>Tobasa Web Server</title><style>";
   content += kMinimalPageCss;
   content += "</style></head><body><div class=\"card\"><div class=\"badge\">HTTP response</div><h1>";
   content += status.reasonWithCode();
   content += "</h1></div></body></html>";

   auto response = context->response();
   response->httpStatus( status );
   response->content(std::move(content));
   response->setHeaderContentType("text/html");

   return http::RequestStatus::handled;
}


/// Handle request to /hello
http::RequestStatus handleHelloPage(const http::HttpContext& context)
{
   using namespace tbs::http;

   auto response = context->response();
   response->addHeader("X-Processed-By", "Request Handler");

   std::string content = "<html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>Tobasa Web Server</title><style>";
   content += kMinimalPageCss;
   content += "</style></head><body><div class=\"card\"><div class=\"badge\">Tobasa</div><h1>Hello World!</h1><p>Welcome to the Tobasa web server sample.</p></div></body></html>";
   response->content(std::move(content));

   response->httpStatus( StatusCode::OK );
   response->setHeaderContentType("text/html");
   response->addCookieHeader(std::make_shared<ResponseCookie>("cookie_test_1", "XX1234567890", 3600));
   response->addCookieHeader(ResponseCookie::remove("cookie_test_2"));
   response->addCookieHeader(std::make_shared<ResponseCookie>("cookie_test_3", true));

   return RequestStatus::handled;
}


/// Handle request to /upload
http::RequestStatus handleUpload(const http::HttpContext& context)
{
   using namespace tbs::http;

   auto response = context->response();
   auto request  = context->request();

   response->addHeader("X-Processed-By", "Request Handler");

   auto requestBody = request->content();
   auto formBody    = request->formBody();

   if ( request->hasMultipart() && request->hasMultipartBody() )
   {
      namespace fs = std::filesystem;

      auto body = request->multipartBody();
      auto part = body->find("profileImage");
      if (part && part->isFile)
      {
         std::error_code filesystemError;
         fs::path uploadFolder;
         if (!createUploadFolder(uploadFolder, filesystemError))
            return statusResult(context, StatusCode::INTERNAL_SERVER_ERROR);

         fs::path savedProfileImage;
         for (const auto& filePart : body->parts())
         {
            if (!filePart || !filePart->isFile)
               continue;

            std::string fileName  = sanitizeUploadFileName(filePart->fileName);

            fs::path originalName(fileName);
            std::string fileStem  = originalName.stem().string();
            std::string extension = originalName.extension().string();
            fs::path destination  = uploadFolder / originalName;

            for (std::size_t duplicate = 1; fs::exists(destination, filesystemError); ++duplicate)
            {
               if (filesystemError)
                  return statusResult(context, StatusCode::INTERNAL_SERVER_ERROR);

               destination = uploadFolder / (fileStem + "_" + std::to_string(duplicate) + extension);
            }

            if (filesystemError)
               return statusResult(context, StatusCode::INTERNAL_SERVER_ERROR);

            if (!path::isPathWithinRoot(destination.string(), uploadFolder.string()))
               return statusResult(context, StatusCode::FORBIDDEN);

            fs::copy_file(filePart->location, destination, fs::copy_options::none, filesystemError);
            if (filesystemError)
               return statusResult(context, StatusCode::INTERNAL_SERVER_ERROR);

            if (filePart == part)
               savedProfileImage = destination;
         }

         if (savedProfileImage.empty())
            return statusResult(context, StatusCode::INTERNAL_SERVER_ERROR);

         auto userNamePart = body->find("userName");
         auto userNotePart = body->find("userNote");
         std::string userName = userNamePart && !userNamePart->isFile ? userNamePart->body : "";
         std::string userNote = userNotePart && !userNotePart->isFile ? userNotePart->body : "";
         Json info;
         info["userName"] = userName;
         info["userNote"] = userNote;

         std::ofstream infoFile(uploadFolder / "info.json", std::ios::out | std::ios::binary | std::ios::trunc);
         if (!infoFile)
            return statusResult(context, StatusCode::INTERNAL_SERVER_ERROR);

         infoFile << info.dump(2) << '\n';
         infoFile.close();
         if (!infoFile)
            return statusResult(context, StatusCode::INTERNAL_SERVER_ERROR);

         // Echo back profile image to client

         // use chunked encoding for file response, or just comment this line to use content-length
         response->useChunkedEncoding(true);

         response->fileContent(savedProfileImage.string());
         response->httpStatus(StatusCode::OK);
         response->setHeaderContentType(part->contentType);

         return RequestStatus::handled;
      }
   }
   else
   {
      std::string content = "<html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>Tobasa Web Server</title><style>";
      content += kMinimalPageCss;
      content += "</style></head><body><div class=\"card\"><div class=\"badge\">Upload</div><h1>Request body</h1><p>";
      content += requestBody;
      content += "</p></div></body></html>";

      response->content(std::move(content));
      response->httpStatus( StatusCode::OK );
      response->setHeaderContentType("text/html");
   }

   return RequestStatus::handled;
}


/// Handle WebSocket request to /websocket_ep ( ws://server/websocket_ep )
http::RequestStatus handleWebsocketEndpoint(const http::HttpContext& context)
{
   auto response = context->response();

   // set a context and establish websocket connection.
   auto wsContext = wsCtxWrapper->wsContext;
   context->webSocketContext(wsContext);
   
   // Note: by default response has empty content and status 200
   
   // Here, we only need to give Http status 200
   response->content("");                       // empty response body
   response->httpStatus( http::StatusCode::OK); // explicitly set status code to 200

   return http::RequestStatus::handled;
}


/// Handle request to WebSocket HTML page /test_websocket
http::RequestStatus handleWebsocket(const http::HttpContext& context)
{
   auto request  = context->request();
   auto response = context->response();

   std::string documentRoot("./wwwroot");

   std::string requestPath = context->request()->path();

   // Request path must be absolute and not contain "..".
   if (requestPath.empty() || requestPath[0] != '/' || requestPath.find("..") != std::string::npos) {
      return statusResult(context, http::StatusCode::FORBIDDEN);
   }

   // If file does not exist, return NOT FOUND
   std::string fullPath = documentRoot + requestPath + ".html";

   if (!path::isPathWithinRoot(fullPath, documentRoot))
      return statusResult(context, http::StatusCode::FORBIDDEN);

   if (!path::exists(fullPath)) {
      return statusResult(context, http::StatusCode::NOT_FOUND);
   }

   response->setHeaderContentType(http::mimetypes::fromExtension("html"));
   response->addHeader("X-Processed-By", "Request Handler");
   response->httpStatus( http::StatusCode::OK);
   response->fileContent(fullPath);

   return http::RequestStatus::handled;
}


/// Handle request to index page /
http::RequestStatus handleIndexPage(const http::HttpContext& context)
{
   auto response = context->response();

   std::string documentRoot("./wwwroot");
   std::string requestPath = context->request()->path();

   // Request path must be absolute and not contain "..".
   if (requestPath.empty() || requestPath[0] != '/' || requestPath.find("..") != std::string::npos) {
      return statusResult(context, http::StatusCode::FORBIDDEN);
   }

   // If path ends in slash (i.e. is a directory) then add "index.html".
   if (requestPath[requestPath.size() - 1] == '/') {
      requestPath += "index.html";
   }

   // Determine the file extension.
   std::size_t lastSlashPos = requestPath.find_last_of("/");
   std::size_t lastDotPos   = requestPath.find_last_of(".");
   std::string extension;
   if (lastDotPos != std::string::npos && lastDotPos > lastSlashPos) {
      extension = requestPath.substr(lastDotPos + 1);
   }

   // If file does not exist, return NOT FOUND
   std::string fullPath = documentRoot + requestPath;
   
   if (!path::isPathWithinRoot(fullPath, documentRoot)) {
      return statusResult(context, http::StatusCode::FORBIDDEN);
   }

   if (! path::exists(fullPath)) {
      return statusResult(context, http::StatusCode::NOT_FOUND);
   }

   // Build http response
   // -------------------------------------------------------
   response->httpStatus( http::StatusCode::OK );
   response->setHeaderContentType(http::mimetypes::fromExtension(extension));

   bool useStringReader = false;
   if (useStringReader)
   {
      // Open the file to send back.
      std::ifstream is(fullPath, std::ios::in | std::ios::binary);
      if (!is)
         return statusResult(context, http::StatusCode::NOT_FOUND);

      std::string content;
      char buffer[8 * 1024];
      while (is.read(buffer, sizeof(buffer)).gcount() > 0)
      {
         content.append(buffer, is.gcount());
      }
      // set content and init data source and reader
      response->content(std::move(content));
   }
   else
   {
      response->fileContent(fullPath);
   }

   return http::RequestStatus::handled;
}


/// Handle request to /browse_uploads
http::RequestStatus handleBrowseUpload(const http::HttpContext& context)
{
   namespace fs = std::filesystem;

   auto response = context->response();
   auto request = context->request();
   const std::string route = "/browse_uploads";
   const std::string requestPath = request->path();
   if (requestPath != route && !util::startsWith(requestPath, route + "/"))
      return statusResult(context, http::StatusCode::NOT_FOUND);

   fs::path uploadsRoot = fs::path(path::executableDir()) / "app_data" / "uploads";

   std::error_code filesystemError;

   fs::create_directories(uploadsRoot, filesystemError);
   if (filesystemError)
      return statusResult(context, http::StatusCode::INTERNAL_SERVER_ERROR);

   std::string relativeText = requestPath.substr(route.size());
   if (!relativeText.empty() && relativeText.front() == '/')
      relativeText.erase(0, 1);

   while (!relativeText.empty() && (relativeText.back() == '/' || relativeText.back() == '\\'))
      relativeText.pop_back();

   fs::path relativePath(relativeText);
   relativePath = relativePath.lexically_normal();
   if (relativePath == ".")
      relativePath.clear();

   fs::path canonicalRoot = fs::weakly_canonical(uploadsRoot, filesystemError);
   if (filesystemError)
      return statusResult(context, http::StatusCode::INTERNAL_SERVER_ERROR);

   fs::path target = fs::weakly_canonical(canonicalRoot / relativePath, filesystemError);
   if (filesystemError)
      return statusResult(context, http::StatusCode::NOT_FOUND);

   if (!path::isPathWithinRoot(target.string(), canonicalRoot.string()))
      return statusResult(context, http::StatusCode::FORBIDDEN);

   fs::file_status targetStatus = fs::symlink_status(target, filesystemError);

   if (filesystemError || !fs::exists(targetStatus))
      return statusResult(context, http::StatusCode::NOT_FOUND);

   if (fs::is_symlink(targetStatus))
      return statusResult(context, http::StatusCode::FORBIDDEN);

   if (fs::is_regular_file(targetStatus))
   {
      std::string extension = target.extension().string();
      if (!extension.empty() && extension.front() == '.')
         extension.erase(0, 1);

      response->httpStatus(http::StatusCode::OK);
      response->setHeaderContentType(http::mimetypes::fromExtension(extension));
      
      response->enableFileRangeResponse(true); // enable Accept-rage for download path
      response->fileContent(target.string());
      
      return http::RequestStatus::handled;
   }

   if (!fs::is_directory(targetStatus))
      return statusResult(context, http::StatusCode::NOT_FOUND);

   std::string content = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>Uploaded Files</title><style>";
   content += kMinimalPageCss;
   content += R"css(
      .card{width:min(820px,100%);text-align:left;}
      .upload-topbar{display:flex;justify-content:space-between;align-items:center;gap:12px;}
      .upload-home{color:#2563eb;text-decoration:none;font-weight:600;}
      .upload-home:hover{text-decoration:underline;}
      .upload-path{margin:0 0 16px;color:#5f6f86;overflow-wrap:anywhere;}
      .upload-nav{margin:0 0 16px;}
      .upload-nav a,.upload-entry{color:#2563eb;text-decoration:none;}
      .upload-nav a:hover,.upload-entry:hover .upload-entry-name{text-decoration:underline;}
      .upload-list{display:grid;gap:8px;}
      .upload-entry{display:grid;grid-template-columns:82px minmax(0,1fr) auto;align-items:center;gap:12px;padding:12px 14px;border:1px solid #d9e2f1;border-radius:10px;background:#f8fbff;}
      .upload-entry-kind{font-size:.8rem;font-weight:700;color:#5f6f86;}
      .upload-entry-name{min-width:0;overflow-wrap:anywhere;color:#1f2a37;}
      .upload-empty{padding:16px;border:1px dashed #d9e2f1;border-radius:10px;color:#5f6f86;}
      @media(max-width:560px){.upload-entry{grid-template-columns:64px minmax(0,1fr);gap:8px}.upload-entry-meta{grid-column:2}}
   )css";
   content += "</style></head><body><main class=\"card\"><div class=\"upload-topbar\"><div class=\"badge\">Upload browser</div><a class=\"upload-home\" href=\"/\">Home</a></div><h1>Uploaded files</h1><p class=\"upload-path\">/";
   content += escapeHtml(relativePath.generic_string());
   content += "</p>";

   if (!relativePath.empty())
   {
      fs::path parentPath = relativePath.parent_path();
      content += "<p class=\"upload-nav\"><a href=\"/browse_uploads";
      if (!parentPath.empty())
      {
         content += "/" + encodeUploadPath(parentPath);
         if (!fs::is_directory(canonicalRoot / parentPath, filesystemError))
            return statusResult(context, http::StatusCode::INTERNAL_SERVER_ERROR);
         content += "/";
      }
      content += "\">Up one level</a></p>";
   }

   content += "<div class=\"upload-list\">";
   bool hasEntries = false;
   for (fs::directory_iterator iterator(target, filesystemError), end; !filesystemError && iterator != end; iterator.increment(filesystemError))
   {
      fs::file_status entryStatus = iterator->symlink_status(filesystemError);
      if (filesystemError)
         return statusResult(context, http::StatusCode::INTERNAL_SERVER_ERROR);

      if (fs::is_symlink(entryStatus))
         continue;

      bool isDirectory = fs::is_directory(entryStatus);
      if (!isDirectory && !fs::is_regular_file(entryStatus))
         continue;

      hasEntries = true;
      fs::path entryRelativePath = relativePath / iterator->path().filename();
      std::string entryName = iterator->path().filename().string();
      content += "<a class=\"upload-entry\" href=\"/browse_uploads/" + encodeUploadPath(entryRelativePath);
      if (isDirectory)
         content += "/";

      content += "\"><span class=\"upload-entry-kind\">";
      content += isDirectory ? "Folder" : "File";
      content += "</span><span class=\"upload-entry-name\">" + escapeHtml(entryName) + "</span><span class=\"upload-entry-meta muted\">";
      
      if (isDirectory)
         content += "Open folder";
      else
      {
         auto fileSize = iterator->file_size(filesystemError);
         if (filesystemError)
            return statusResult(context, http::StatusCode::INTERNAL_SERVER_ERROR);

         content += std::to_string(fileSize) + " bytes";
      }
      content += "</span></a>";
   }

   if (filesystemError)
      return statusResult(context, http::StatusCode::INTERNAL_SERVER_ERROR);

   if (!hasEntries)
      content += "<div class=\"upload-empty\">This folder is empty.</div>";

   content += "</div></main></body></html>";
   response->httpStatus(http::StatusCode::OK);
   response->setHeaderContentType("text/html");
   response->content(std::move(content));

   return http::RequestStatus::handled;
}


/// Server request handler
http::RequestStatus handleServerRequest(const http::HttpContext& context)
{
   auto request = context->request();
   if ( request->path()      == "/hello" ) 
   {
      return handleHelloPage(context);
   }
   else if ( request->path() == "/upload" )
   {
      if (request->method() != "POST")
      {
         auto response = context->response();
         response->addHeader("Allow", "POST");
         return statusResult(context, http::StatusCode::METHOD_NOT_ALLOWED);
      }
      return handleUpload(context);
   }
   else if ( request->path() == "/websocket_ep" )    // websocket endpoint
   {  
      return handleWebsocketEndpoint(context);
   }
   else if ( request->path() == "/test_websocket" )  // websocket html page
   {
      return handleWebsocket(context);
   }
   else if ( request->path() == "/browse_uploads" || util::startsWith(request->path(), "/browse_uploads/") )
   {
      return handleBrowseUpload(context);
   }
   else 
   {
      return handleIndexPage(context);
   }
}


void runServerOnThreadPool(asio::io_context& ioContext, std::vector<std::thread>& threadPool, size_t poolSize)
{
   try
   {
      auto work = asio::make_work_guard(ioContext);
      for (std::size_t i = 0; i < poolSize; ++i)
      {
         threadPool.push_back(
            std::thread{ [&] {
               ioContext.run();
            }}
         );
      }
   }
   catch (const std::exception&)
   {
      ioContext.stop();
      for (auto & thread : threadPool)
      {
         if ( thread.joinable() )
            thread.join();
      }
      throw;
   }

   // Wait for all threads in the pool to exit.
   for (auto & thread : threadPool) {
      thread.join();
   }
}


void runHttpServer()
{
   using namespace tbs;

   wsCtxWrapper = std::make_unique<WebSocketCtxWrapper>();

   // server running context
   asio::io_context ioContext;
   std::vector<std::thread> threadPool;
   size_t ioPoolSize = 4;

   // logger
   log::StdoutLogger logger;
   logger.setLevel(log::Level::TraceMask);

   // settings for http server
   http::Settings httpSetting;
   httpSetting
      .logVerbose(false)
      .port(8084)
      .address("0.0.0.0")
      .maxRequestsPerConnection(0)      // default 100, max 34464, 0 to disable check
      .timeoutRead(10)                  // default 60s, min 10s, max 1 hour. 0 converted to HTTP_TIMEOUT_MAX_ALLOWED
      .timeoutWrite(60)                 // default 60s, min 10s, max 1 hour. 0 converted to HTTP_TIMEOUT_MAX_ALLOWED
      .timeoutProcessing(3600)          // default 120s, min 10s, max 1 hour. 0 converted to HTTP_TIMEOUT_MAX_ALLOWED
      .readBufferSize(1024*32)          // default 64KB, min 16 KB, max 8 MB
      .sendBufferSize(1024*32)          // default 64KB, min 16 KB, max 8 MB
      .maxHeaderSize(1024*1024)         // default 64KB, min 16 KB, max 1 MB
      .temporaryDir("./tmp")
      ;

   // setting for https server
   http::SettingsTls tlsSetting;
   tlsSetting
      .serverMode(true)
      .certificateChainFile( "localhost.crt" )
      .privateKeyFile( "localhost.key" )
      .tmpDhFile( "dh2048.pem" )
#ifdef TOBASA_HTTP_USE_HTTP2
      .http2Enabled(true)
      .logVerboseHttp2(false)
#endif
      .logVerbose(false)
      .port(8085)
      .address("0.0.0.0")
      .maxRequestsPerConnection(100)    // default 100, max 34464, 0 to disable check
      .timeoutRead(10)                  // default 60s, min 10s, max 1 hour. 0 converted to HTTP_TIMEOUT_MAX_ALLOWED
      .timeoutWrite(60)                 // default 60s, min 10s, max 1 hour. 0 converted to HTTP_TIMEOUT_MAX_ALLOWED
      .timeoutProcessing(3600)          // default 120s, min 10s, max 1 hour. 0 converted to HTTP_TIMEOUT_MAX_ALLOWED
      .readBufferSize(1024*32)          // default 64KB, min 16 KB, max 8 MB
      .sendBufferSize(1024*32)          // default 64KB, min 16 KB, max 8 MB
      .maxHeaderSize(1024*1024)         // default 64KB, min 16 KB, max 1 MB
      .temporaryDir("./tmp")
      ;


   http::PlainServerDefault serverHttp(ioContext, std::move(httpSetting), logger);
   // set server request handler
   serverHttp.requestHandler(handleServerRequest);


   http::SecureServerDefault serverHttps(ioContext, std::move(tlsSetting), logger);
   // set server request handler
   serverHttps.requestHandler(
      [&](const http::HttpContext& context)
      {
         return handleServerRequest(context);
      });


   // Starts server in async
   // -------------------------------------------------------
   std::exception_ptr exceptionCaught;
   asio::signal_set breakSignals{ ioContext, SIGINT };

   breakSignals.async_wait(
      [&](const asio::error_code & ec, int)
      {
         if (!ec)
         {
            asio::post(
               serverHttp.executor(),
               [&] {
                  try
                  {
                     serverHttp.stop();
                     serverHttps.stop();

                     wsCtxWrapper.reset();

                     if ( ioPoolSize > 0)
                        ioContext.stop();
                  }
                  catch (...)
                  {
                     ioContext.stop();
                     exceptionCaught = std::current_exception();
                  }
               });
         }
      });

   asio::post(
      serverHttp.executor(),
      [&] {
         try
         {
            serverHttp.start();
            serverHttps.start();
         }
         catch (...)
         {
            ioContext.stop();
            exceptionCaught = std::current_exception();
         }
      });

   if (ioPoolSize > 0)
      runServerOnThreadPool(ioContext, threadPool, ioPoolSize);
   else
      ioContext.run();

   // If an error was detected it should be propagated.
   if (exceptionCaught)
      std::rethrow_exception( exceptionCaught );
}


int main(int argc, char* argv[])
{
   try
   {
      if (! DateTime::initTimezoneData())
         return 1;

      std::cout << "TOBASA HTTP Server\n";
      runHttpServer();
   }
   catch (std::exception& ex) {
      std::cout << ex.what() << "\n";
   }
   catch(...) {
      std::cout << "Exception occured";
   }

   return 0;
}