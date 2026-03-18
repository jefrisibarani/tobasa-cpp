#include <fstream>
#include <filesystem>
#include <tobasa/datetime.h>
#include <tobasa/path.h>
#include <tobasa/format.h>
#include <tobasa/logger.h>
#include <tobasa/file_reader.h>
#include <tobasa/string_reader.h>
#include <tobasa/util.h>
#include <tobasahttp/server/http_server.h>
#include <tobasahttp/mimetypes.h>

#include <tobasa/span.h>

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

   std::shared_ptr<tbs::http::WebSocketContext> wsContext;

   void createWebSocketContext();
};

using namespace tbs;

static std::unique_ptr<WebSocketCtxWrapper> wsCtxWrapper = nullptr;
static std::unique_ptr<http::parser::MultipartParser> multipartParser = nullptr;

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

   wsContext->onMessage = 
      [wsContext=wsContext->shared_from_this()]
      (http::WebSocketPtr conn, const std::string& message)
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

bool copyFileToDisk(const std::string& destFolder, const std::string& fileName, const std::string& sourcePath, std::string &outError)
{
   namespace fs = std::filesystem;
   try
   {
      fs::path source = sourcePath;
      fs::path destination = tbsfmt::format( "{}{}{}", destFolder, tbs::path::SEPARATOR, fileName );
      fs::copy_file(source, destination, fs::copy_options::overwrite_existing);
   }
   catch (const std::exception& ex)
   {
      outError = ex.what();
      return false;
   }
   
   return true;
}

/// Create simple status response html page
tbs::http::RequestStatus statusResult(const tbs::http::HttpContext& context, tbs::http::StatusCode statusCode)
{
   using namespace tbs;

   http::HttpStatus status(statusCode);

   std::string content = "<html><head><title>Tobasa Web Server</title></head>";
   content.append("<body><h3>");
   content.append(status.reasonWithCode());
   content.append("</h3></body></html>");

   auto response = context->response();
   response->httpStatus( status );
   response->content(std::move(content));
   response->setHeaderContentType("text/html");

   return http::RequestStatus::handled;
}

/// Handle request to /hello
tbs::http::RequestStatus handleHelloPage(const tbs::http::HttpContext& context)
{
   using namespace tbs::http;

   auto response = context->response();
   response->addHeader("X-Processed-By", "Request Handler");

   std::string content = "<html><head><title>Tobasa Web Server</title></head>";
   content.append("<body><h3>");
   content.append("Hello World!");
   content.append("</h3></body></html>");
   
   response->content(std::move(content));
   response->httpStatus( StatusCode::OK );
   response->setHeaderContentType("text/html");
   
   response->addCookieHeader(std::make_shared<ResponseCookie>("cookie_test_1", "XX1234567890", 3600));

   // response->addCookieHeader(ResponseCookie::create("cookie_test_2", "SSSSSSSSSSSSSSS", 3600));

   // std::string cookie2Str = ResponseCookie("cookie_test_3", "xczxcsdsdsdxcz", 3600)
   //                              .path("/").domain("localhost").secure(true).httpOnly(true).sameSite("Strict").toString();
   // response->addHeader("Set-Cookie", cookie2Str);

   response->addCookieHeader(ResponseCookie::remove("cookie_test_2"));
   response->addCookieHeader(std::make_shared<ResponseCookie>("cookie_test_3", true));

   return RequestStatus::handled;
}

/// Handle request to /test/upload
tbs::http::RequestStatus handleUpload(const tbs::http::HttpContext& context)
{
   using namespace tbs::http;

   auto response = context->response();
   auto request  = context->request();

   response->addHeader("X-Processed-By", "Request Handler");

   auto requestBody = request->content();
   auto formBody    = request->formBody();

   if ( request->hasMultipart() && request->hasMultipartBody() )
   {
      auto body = request->multipartBody();
      auto part = body->find("profileImage");
      if (part && part->isFile)
      {
         // output back the file to client
         response->useChunkedEncoding(true);    // use chunked encoding for file response, or just comment this line to use content-length
         response->fileContent(part->location);
         response->httpStatus(StatusCode::OK);
         response->setHeaderContentType(part->contentType);

         return RequestStatus::handled;
      }
   }
   else
   {
      std::string content = "<html><head><title>Tobasa Web Server</title></head>";
      content.append("<body><h3>");
      content.append(requestBody);
      content.append("</h3></body></html>");

      response->content(std::move(content));
      response->httpStatus( StatusCode::OK );
      response->setHeaderContentType("text/html");
   }

   return RequestStatus::handled;
}

/// Handle WebSocket request to /websocket_ep ( ws://server/websocket_ep )
tbs::http::RequestStatus handleWebsocketEndpoint(const tbs::http::HttpContext& context)
{
   using namespace tbs;

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
tbs::http::RequestStatus handleWebsocket(const tbs::http::HttpContext& context)
{
   using namespace tbs;

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
tbs::http::RequestStatus handleIndexPage(const tbs::http::HttpContext& context)
{
   using namespace tbs;

   auto response = context->response();

   std::string documentRoot("./wwwroot");
   std::string requestPath = context->request()->path();

   // Request path must be absolute and not contain "..".
   if (requestPath.empty() || requestPath[0] != '/' || requestPath.find("..") != std::string::npos) {
      return statusResult(context, http::StatusCode::FORBIDDEN);
   }

   if (util::startsWith(requestPath, "/uploads/"))
      documentRoot = "./data"; // serve from uploads folder

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

#if 0 // -------------------------------------------------------
         auto dataSource = std::make_shared<http::ResponseDataSource>(http::ResponseDataType::file);
         dataSource->dataReader = std::make_unique<FileReader>(fullPath);
         dataSource->filePath   = fullPath;
         dataSource->connect(response); // Replace default datasource
         
         // Set writer callback to read file content
         dataSource->writerCallback2(

      #ifdef TOBASA_HTTP2_WRITE_RESPONSE_NO_COPY_DATA
            [dataSource=std::move(dataSource)](asio::streambuf* sendBuffer, size_t length)
      #else
            [dataSource=std::move(dataSource)](uint8_t* buffer, size_t length, uint32_t* dataFlags)
      #endif
            {
               if (! dataSource->dataReader->isOpen()) 
               {
                  std::cerr << "Error opening file: " << dataSource->filePath << std::endl;
                  return (int64_t)-1;
               }

               auto readCount = std::min(length, dataSource->readLeft);
               auto startAt   = dataSource->dataSize - dataSource->readLeft;

      #ifdef TOBASA_HTTP2_WRITE_RESPONSE_NO_COPY_DATA
               std::vector<uint8_t> buffer(readCount); // Reserve space in the buffer

               std::streamsize bytesRead = dataSource->dataReader->readAt(startAt, buffer.data(), static_cast<std::streamsize>(readCount));
               dataSource->readLeft -= readCount;

               // append data to send buffer
               std::ostream outStream(sendBuffer);
               outStream.write(reinterpret_cast<const char*>(buffer.data()), readCount);
      #else
               std::streamsize bytesRead = dataSource->dataReader->readAt(startAt, buffer, static_cast<std::streamsize>(readCount));
               dataSource->readLeft -= readCount;

               if (dataSource->readLeft == 0) {
                  *dataFlags |= 1; // same as NGHTTP2_DATA_FLAG_EOF;
               }
      #endif
               std::cerr << "__ writerCallback2 readCount : " << readCount << std::endl;
               return static_cast<int64_t>(readCount);
            });
#endif // -------------------------------------------------------

   }

   return http::RequestStatus::handled;
}


/// Server request handler
tbs::http::RequestStatus handleServerRequest(const tbs::http::HttpContext& context)
{
   auto request = context->request();
   if ( request->path()      == "/hello" )
      return handleHelloPage(context);
   else if ( request->path() == "/upload" )
      return handleUpload(context);
   else if ( request->path() == "/websocket_ep" )   // websocket endpoint
      return handleWebsocketEndpoint(context);
   else if ( request->path() == "/test_websocket" ) // websocket html page
      return handleWebsocket(context);
   else
      return handleIndexPage(context);
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

   // We have to set log target for tbs::Logger
   Logger::setTarget(new log::CoutLogSink()) ;
   Logger::disableLogging();

   // logger
   log::StdoutLogger logger;
   logger.setLevel(log::Level::TraceMask);

   // settings for http server
   http::Settings httpSetting;
   httpSetting
      .logVerbose(false)   
      .port(8084)
      .address("0.0.0.0")
      .logVerbose(true)
      .maxRequestsPerConnection(0)      // default 100,  max 10000, set to 0 to disable limit
      .timeoutRead(10)                  // default 60s,  min 10s, max 1 hour, set value to 0 disable limit
      .timeoutWrite(60)                 // default 60s,  min 10s, max 1 hour, set value to 0 disable limit
      .timeoutProcessing(3600)          // default 120s, min 10s, max 1 hour, set value to 0 disable limit
      .readBufferSize(1024*32)          // default 64KB, min 1KB, max 8 MB
      .sendBufferSize(1024*32)          // default 64KB, min 1KB, max 8 MB
      .maxHeaderSize(1024*1024)         // default 64KB, min 1KB, max 1 MB
      .enableMultipartParsing(true)
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
      .logVerbose(true)
      .port(8085)
      .address("0.0.0.0")
      .maxRequestsPerConnection(100)    // default 100,  max 10000, set to 0 to disable limit
      .timeoutRead(10)                  // default 60s,  min 10s, max 1 hour, set value to 0 disable limit
      .timeoutWrite(60)                 // default 60s,  min 10s, max 1 hour, set value to 0 disable limit
      .timeoutProcessing(3600)          // default 120s, min 10s, max 1 hour, set value to 0 disable limit
      .readBufferSize(1024*32)          // default 64KB, min 1KB, max 8 MB
      .sendBufferSize(1024*32)          // default 64KB, min 1KB, max 8 MB
      .maxHeaderSize(1024*1024)         // default 64KB, min 1KB, max 1 MB
      .enableMultipartParsing(true)
      .temporaryDir("./tmp")
      ;


   http::PlainServerDefault serverHttp(ioContext, std::move(httpSetting), logger);
   // set server request handler
   serverHttp.requestHandler(
      [&](const http::HttpContext& context)
      {
         return handleServerRequest(context);
      });


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
      if (! tbs::DateTime::initTimezoneData())
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