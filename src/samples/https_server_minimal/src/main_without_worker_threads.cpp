#include <tobasa/datetime.h>
#include <tobasa/logger.h>
#include "tobasahttp/server/http_server.h"

void runHttpsServer()
{
   using namespace tbs;

   // server running context
   asio::io_context ioContext;

   // We have to set log target for tbs::Logger
   tbs::Logger::setTarget(new tbs::log::CoutLogSink()) ;
   // logger
   log::StdoutLogger logger;
   logger.setLevel(log::Level::TraceMask);
   
   // setting for https server
   http::SettingsTls settings("0.0.0.0", 8085);
   settings.logVerbose(true);
   settings.maxRequestsPerConnection(0); // disable 

   http::SecureServerDefault serverHttps(ioContext, std::move(settings), logger);

   // set server request handler
   serverHttps.requestHandler(
      [&](const http::HttpContext& context)
      {
         context->response()->addHeader("X-Processed-By", "Request Handler");

         std::string content("<html><head><title>Tobasa Web Server</title></head>");
         content.append("<body><h3>");
         content.append("Hello World!");
         content.append("</h3></body></html>");

         context->response()->content(std::move(content));
         context->response()->httpStatus( tbs::http::StatusCode::OK );
         context->response()->setHeaderContentType("text/html");

         return tbs::http::RequestStatus::handled;
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
      serverHttps.executor(),
      [&] {
         try
         {
            serverHttps.start();
         }
         catch (...)
         {
            ioContext.stop();
            exceptionCaught = std::current_exception();
         }
      });

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