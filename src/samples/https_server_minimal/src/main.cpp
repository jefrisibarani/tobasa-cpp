#include <tobasa/datetime.h>
#include <tobasa/logger.h>
#include "tobasahttp/server/http_server.h"


/// Handle request 
tbs::http::RequestStatus realRequestHandler(const tbs::http::HttpContext& context)
{
   using namespace tbs::http;

   auto request = context->request();
   if ( request->path() == "/db" )
   {
      // Simulate blocking DB Call
      std::this_thread::sleep_for(std::chrono::milliseconds(15000));
   }

   auto response = context->response();

   response->addHeader("X-Processed-By", "Request Handler");
   std::string content = "<html><head><title>Tobasa Web Server</title></head>";
   content.append("<body><h3>");
   content.append( request->path() + " Hello World! ");
   content.append("</h3></body></html>");

   response->content(std::move(content));
   response->httpStatus( StatusCode::OK );
   response->setHeaderContentType("text/html");
   response->addCookieHeader(std::make_shared<ResponseCookie>("cookie_test_1", "XX1234567890", 3600));
   response->addCookieHeader(ResponseCookie::remove("cookie_test_2"));
   response->addCookieHeader(std::make_shared<ResponseCookie>("cookie_test_3", true));

   return RequestStatus::handled;
}


void runHttpsServer()
{
   using namespace tbs;

   size_t ioThreads     = std::thread::hardware_concurrency();
   if (ioThreads == 0) 
      ioThreads = 4;
   size_t workerThreads = std::thread::hardware_concurrency();
   if (workerThreads == 0) 
      workerThreads = 4;

   // server running context
   asio::io_context ioContext;

   // Worker pool for blocking operations
   asio::thread_pool workerPool(workerThreads);

   // We have to set log target for tbs::Logger
   tbs::Logger::setTarget(new tbs::log::CoutLogSink()) ;
   // logger
   log::StdoutLogger logger;
   logger.setLevel(log::Level::TraceMask);
   
   tbs::Logger::logI("Starting Application");

   // setting for https server
   http::SettingsTls settings("0.0.0.0", 8085);
   settings.logVerbose(true);
   settings.maxRequestsPerConnection(0); // disable 

   http::SecureServerDefault serverHttps(ioContext, std::move(settings), logger);

   // set server request handler
   serverHttps.requestHandler(
      [&](const http::HttpContext& context)
      {
         asio::post(workerPool,
            [&,ctx=context]()
            {
               try
               {
                  auto resultStatus = realRequestHandler(ctx);

                  // Return to IO thread to send response
                  asio::post(ioContext,
                     [&,ctx=ctx,resultStatus]()
                     {
                        ctx->complete(resultStatus);
                     });
               }
               catch (...)
               {
                  asio::post(ioContext,
                     [&,ctx=ctx]()
                     {
                        ctx->response()->httpStatus( tbs::http::StatusCode::INTERNAL_SERVER_ERROR );
                        ctx->response()->setHeaderContentType("text/html");
                        ctx->complete(tbs::http::RequestStatus::handled);
                     });
               }
            });

         return tbs::http::RequestStatus::async; // important
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
               serverHttps.executor(),
               [&] {
                  try
                  {
                     serverHttps.stop();
                     ioContext.stop();
                     workerPool.stop();
                  }
                  catch (...)
                  {
                     ioContext.stop();
                     workerPool.stop();
                     exceptionCaught = std::current_exception();
                  }
               });
         }
      });

   asio::post(
      serverHttps.executor(),
      [&] {
         try
         {
            serverHttps.start();
         }
         catch (...)
         {
            ioContext.stop();
            workerPool.stop();
            exceptionCaught = std::current_exception();
         }
      });

   ioContext.run();

   // If an error was detected it should be propagated.
   if (exceptionCaught)
      std::rethrow_exception( exceptionCaught );

   workerPool.join();
}


int main(int argc, char* argv[])
{
   try
   {
      if (! tbs::DateTime::initTimezoneData())
         return 1;

      std::cout << "TOBASA HTTP Server Minimal\n";
      runHttpsServer();
   }
   catch (std::exception& ex) {
      std::cout << ex.what() << "\n";
   }
   catch(...) {
      std::cout << "Exception occured";
   }

   return 0;
}