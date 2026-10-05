#include <csignal>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <utility>

#include <tobasa/base64.h>
#include <tobasa/datetime.h>
#include <tobasa/logger.h>
#include "tobasahttp/server/http_server.h"
#include "tobasahttp/multipart_parser.h"

using namespace tbs;

static constexpr char kMinimalPageCss[] = R"css(
   body{margin:0;min-height:100vh;padding:24px 16px;box-sizing:border-box;font-family:Segoe UI,Arial,sans-serif;background:linear-gradient(180deg,#f8fbff 0%,#f3f6fb 100%);color:#1f2a37;display:flex;align-items:center;justify-content:center;}
   .card{width:min(560px,100%);box-sizing:border-box;background:#fff;border:1px solid #d9e2f1;border-radius:18px;padding:40px 32px;box-shadow:0 12px 30px rgba(15,23,42,.08);text-align:center;}
   h1{margin:0 0 12px;font-size:2rem;}
   p{margin:0;font-size:1rem;color:#5f6f86;line-height:1.6;}
   .badge{display:inline-flex;align-items:center;margin-bottom:16px;padding:7px 10px;border-radius:999px;background:#edf2ff;color:#2563eb;font-size:.82rem;font-weight:700;}
   )css";

static std::string escapeHtml(const std::string& value)
{
   std::string escaped;
   for (char character : value)
   {
      switch (character)
      {
         case '&':  escaped += "&amp;";  break;
         case '<':  escaped += "&lt;";   break;
         case '>':  escaped += "&gt;";   break;
         case '"': escaped += "&quot;"; break;
         case '\'': escaped += "&#39;";  break;
         default:   escaped += character; break;
      }
   }
   return escaped;
}

/// Create simple status response html page
http::RequestStatus statusResult(const http::HttpContext& context, http::StatusCode statusCode, const std::string& detail = {})
{
   http::HttpStatus status(statusCode);
   std::string content = "<html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>Tobasa Web Server</title><style>";
   content += kMinimalPageCss;
   content += "</style></head><body><div class=\"card\"><div class=\"badge\">HTTP response</div><h1>";
   content += status.reasonWithCode();
   content += "</h1>";
   if (!detail.empty())
      content += "<p>" + escapeHtml(detail) + "</p>";
   content += "</div></body></html>";
   auto response = context->response();
   response->httpStatus( status );
   response->content(std::move(content));
   response->setHeaderContentType("text/html");
   return http::RequestStatus::handled;
}



// ------------------------------------------------
// Middlewares
// ------------------------------------------------

http::RequestStatus middlewareExceptionHandler(const http::HttpContext& context, const http::RequestHandler& next)
{
   try
   {
      return next(context);
   }
   catch (const std::exception& ex)
   {
      return statusResult(context, http::StatusCode::INTERNAL_SERVER_ERROR, ex.what());
   }
   catch (...)
   {
      return statusResult(context, http::StatusCode::INTERNAL_SERVER_ERROR, "An unknown error occurred while processing the request.");
   }
}

http::RequestStatus middlewareMultipart(const http::HttpContext& context, http::RequestHandler nextHandler)
{
   auto request = context->request();

   if (!request->hasMultipart())
      return nextHandler(context);

   if (!context->getBodyReader())
      throw std::runtime_error("Missing MultipartBodyReader for multipart request");

   auto parser = std::make_shared<http::parser::MultipartParser>("./tmp");
   auto contentLength = context->request()->contentLength();
   parser->contentLength(contentLength);

   if (context->getBodyReader()->chunkedMultipart())
      parser->chunkedMultipart(true);

   auto ctype = context->request()->contentType();
   if (!ctype.empty())
   {
      http::MediaType media;
      media.parse(ctype);
      if (media.valid() && tbs::util::toLower(media.fullType()) == "multipart/form-data")
      {
         auto boundary = media.find("boundary");
         if (boundary && boundary->valid() && parser->applyBoundary(boundary->value()))
            context->request()->multipartBody(std::make_unique<http::MultipartBody>());
         else
            throw std::runtime_error("Invalid multipart boundary");
      }
   }

   auto nextHandlerForAsync = std::move(nextHandler);
   auto dataHandler =
      [weakContext = std::weak_ptr<http::Context>{context},
       nextHandler = std::move(nextHandlerForAsync),
       mparser = std::move(parser)](const uint8_t* data, size_t totalData)
      {
         auto currentContext = weakContext.lock();
         if (!currentContext)
            return http::parser::Info{false, "HTTP context is no longer available", 0, {}, 0};

         auto info = mparser->parse(data, totalData);
         if (info.success())
         {
            if ( mparser->done() )
            {
               info.message("multipart-done");

               currentContext->request()->multipartBody(std::move(mparser->multipartBody()));
               currentContext->getBodyReader()->done(true);
               if (nextHandler)
               {
                  // resume pipeline
                  auto nextStatus = nextHandler(currentContext);
                  currentContext->complete(nextStatus);
               }
               else
                  currentContext->complete(); // default ends with RequestStatus::handled
            }
         }

         return info;
      };

   context->getBodyReader()->read(std::move(dataHandler));

   // Instead of next(context), we return async status.
   // This way ServerConnection will not write a response immediately, 
   // But later after we call context->complete() 
   return http::RequestStatus::async;
}

// Build a request pipeline.
//
// - finalHandler is the last step in the chain and represents the real route logic.
// - each middleware wraps the next step.
// - the first middleware in the list runs first and can choose to:
//     * stop and return a response immediately
//     * inspect or modify the request
//     * call next(context) to continue to the next layer
//
// In this sample the effective call order is:
//   middlewareExceptionHandler -> middlewareMultipart -> handleRoute
static http::RequestHandler composeMiddlewares(
   std::initializer_list<http::RequestHandlerChained> middlewares,
   http::RequestHandler finalHandler)
{
   http::RequestHandler chain = std::move(finalHandler);

   for (auto it = middlewares.begin(); it != middlewares.end(); ++it)
   {
      auto middleware = *it;
      auto next = chain;
      chain = [middleware, next](const http::HttpContext& context)
      {
         return middleware(context, next);
      };
   }

   return chain;
}

// ------------------------------------------------
// Route handler
// ------------------------------------------------

http::RequestStatus handleRouteDefault(const http::HttpContext& context)
{
   using namespace tbs::http;
   auto response = context->response();
   response->addHeader("X-Processed-By", "Request Handler");
   std::string content = "<html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>Tobasa Web Server</title><style>";
   content += kMinimalPageCss;
   content += "</style></head><body><div class=\"card\"><div class=\"badge\">Tobasa</div><h1>Hello World!</h1><p>Welcome to the Tobasa web server sample.</p><p><a href=\"/form_upload\">Try the multipart profile upload</a></p></div></body></html>";
   response->content(std::move(content));
   response->httpStatus( StatusCode::OK );
   response->setHeaderContentType("text/html");
   return RequestStatus::handled;
}


http::RequestStatus handleRouteFormUpload(const http::HttpContext& context)
{
   auto request = context->request();
   auto response = context->response();

   if (request->method() != "GET")
   {
      response->addHeader("Allow", "GET");
      return statusResult(context, http::StatusCode::METHOD_NOT_ALLOWED);
   }

   std::string content = "<html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>Multipart Upload</title><style>";
   content += kMinimalPageCss;
   content += "</style></head><body><main class=\"card\"><div class=\"badge\">Multipart</div><h1>Upload profile</h1><form action=\"/upload\" method=\"post\" enctype=\"multipart/form-data\"><p><label for=\"userName\">User Name</label><br><input id=\"userName\" name=\"userName\" type=\"text\" autocomplete=\"name\" required></p><p><label for=\"profileImage\">Profile image</label><br><input id=\"profileImage\" name=\"profileImage\" type=\"file\" accept=\"image/*\" required></p><p><button type=\"submit\">Upload</button></p></form></main></body></html>";

   response->content(std::move(content));
   response->httpStatus(http::StatusCode::OK);
   response->setHeaderContentType("text/html");
   return http::RequestStatus::handled;

}


http::RequestStatus handleRouteUpload(const http::HttpContext& context)
{
   auto request = context->request();
   auto body = request->multipartBody();
   if (!request->hasMultipartBody() || !body)
      return statusResult(context, http::StatusCode::BAD_REQUEST, "No parsed multipart body was attached to this request.");

   auto userNamePart = body->find("userName");
   auto profileImagePart = body->find("profileImage");

   std::string userName = userNamePart && !userNamePart->isFile ? userNamePart->body : "";
   std::string profileImageName = profileImagePart && profileImagePart->isFile ? profileImagePart->fileName : "";
   std::string profileImageLocation = profileImagePart && !profileImagePart->location.empty() ? profileImagePart->location : "";
   std::string imageData;
   std::string imageContentType;
   if (profileImagePart && profileImagePart->isFile && !profileImageLocation.empty())
   {
      imageContentType = profileImagePart->contentType;
      if (imageContentType == "image/png" || imageContentType == "image/jpeg" ||
          imageContentType == "image/gif" || imageContentType == "image/webp")
      {
         std::ifstream imageFile(profileImageLocation, std::ios::binary);
         std::string imageBytes((std::istreambuf_iterator<char>(imageFile)), std::istreambuf_iterator<char>());
         if (imageFile.is_open() && !imageFile.bad() && !imageBytes.empty())
            imageData = base64::encode(imageBytes);
      }
   }

   std::string content = "<html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>Multipart Upload</title><style>";
   content += kMinimalPageCss;
   content += "</style></head><body><div class=\"card\"><div class=\"badge\">Multipart</div><h1>Upload received</h1><p><strong>User Name:</strong> ";
   content += escapeHtml(userName.empty() ? "(not provided)" : userName);
   content += "</p><p><strong>Profile Image:</strong> ";
   content += escapeHtml(profileImageName.empty() ? "(not provided)" : profileImageName);
   content += "</p>";
   if (!profileImageLocation.empty())
   {
      content += "<p><strong>Saved path:</strong> ";
      content += escapeHtml(profileImageLocation);
      content += "</p>";
   }
   if (!imageData.empty())
   {
      content += "<p><img alt=\"Uploaded profile image\" style=\"max-width:100%;max-height:360px;object-fit:contain;border-radius:8px\" src=\"data:";
      content += escapeHtml(imageContentType);
      content += ";base64,";
      content += imageData;
      content += "\"></p>";
   }
   content += "<p>Parsed ";
   content += std::to_string(body->parts().size());
   content += " multipart parts.</p></div></body></html>";

   auto response = context->response();
   response->content(std::move(content));
   response->httpStatus(http::StatusCode::OK);
   response->setHeaderContentType("text/html");

   return http::RequestStatus::handled;
}

// This is the actual business logic for the request.
// The middleware chain runs before it: the exception wrapper catches failures,
// the multipart middleware prepares upload data, and then this function decides
// which page or action to send back. In other words, the route is the final
// step in the pipeline, not the first one.
http::RequestStatus handleRoute(const http::HttpContext& requestContext)
{
   auto request = requestContext->request();

   if (request->path() == "/upload")
   {
      if (request->method() != "POST")
      {
         auto response = requestContext->response();
         response->addHeader("Allow", "POST");
         return statusResult(requestContext, http::StatusCode::METHOD_NOT_ALLOWED);
      }
      return handleRouteUpload(requestContext);
   }
   else if (request->path() == "/form_upload")
   {
      return handleRouteFormUpload(requestContext);
   }

   return handleRouteDefault(requestContext);
}


// ------------------------------------------------
// HTTP Server request handler
// ------------------------------------------------

// Request flow is:
//   1. middlewareExceptionHandler runs first and wraps the whole pipeline
//   2. middlewareMultipart checks the upload request and parses multipart data
//   3. handleRoute decides which page or upload action to serve
//
// we preprocess the request, then route it, and any exception in
// the middle is caught and turned into an HTTP error page.
http::RequestStatus handleHttpRequest(const http::HttpContext& context)
{
   auto pipeline = composeMiddlewares(
                     {
                        middlewareMultipart,
                        middlewareExceptionHandler
                     },
                     handleRoute );

   return pipeline(context);
}


int main()
{
   try
   {
      using namespace tbs;

      if (!DateTime::initTimezoneData())
         return 1;

      asio::io_context ioContext;

      Logger::setTarget(new log::CoutLogSink());
      log::StdoutLogger logger;
      logger.setLevel(log::Level::TraceMask);

      http::SettingsTls settings;
      settings
         .logVerbose(true)
#ifdef TOBASA_HTTP_USE_HTTP2
         .http2Enabled(true)
         .logVerboseHttp2(true)
#endif
         .port(8085)
         .address("0.0.0.0")
         .certificateChainFile("localhost.crt")
         .privateKeyFile("localhost.key")
         .tmpDhFile("dh2048.pem")
         .enableMultipartParsing(false)
         .temporaryDir("./tmp")
         ;

      http::SecureServerDefault serverHttps(ioContext, std::move(settings), logger);
      serverHttps.requestHandler(handleHttpRequest);

      asio::signal_set breakSignals{ ioContext, SIGINT };
      breakSignals.async_wait(
         [&](const asio::error_code& error, int)
         {
            if (!error)
               serverHttps.stop();
         });

      serverHttps.start();
      ioContext.run();
   }
   catch (const std::exception& ex)
   {
      std::cout << ex.what() << "\n";
      return 1;
   }
   catch (...)
   {
      std::cout << "Exception occured\n";
      return 1;
   }

   return 0;
}
