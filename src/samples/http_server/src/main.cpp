#include <tobasa/datetime.h>
#include <tobasa/logger.h>
#include <tobasa/util.h>
#include <tobasahttp/server/http_server.h>

#include "handlers.h"

using namespace tbs;

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
   else if (request->path() == "/raw_upload")
   {
      if (request->method() != "PUT")
      {
         auto response = context->response();
         response->addHeader("Allow", "PUT");
         return statusResult(context, http::StatusCode::METHOD_NOT_ALLOWED);
      }
      return handleRawUpload(context);
   }
   else if (util::startsWith(request->path(), "/raw_uploads/"))
   {
      if (request->method() != "GET")
      {
         auto response = context->response();
         response->addHeader("Allow", "GET");
         return statusResult(context, http::StatusCode::METHOD_NOT_ALLOWED);
      }
      return handleRawUploadDownload(context);
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

   initializeWebSocketContext();

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
      .useCompression(true)
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
#ifdef TOBASA_HTTP_USE_HTTP2
      .http2Enabled(true)
      .logVerboseHttp2(false)
#endif
      .logVerbose(false)
      .useCompression(true)
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

                     resetWebSocketContext();

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