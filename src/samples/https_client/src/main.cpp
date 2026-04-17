#include <tobasa/datetime.h>
//#include <tobasa/logger.h>  // for tbs::Logger
#include "tobasahttp/client/http_client.h"

void runClient()
{
   using namespace tbs;

   // set actual log target for tbs::logger
   //tbs::Logger::setTarget(new tbs::log::CoutLogSink());
   //TobasaLogger logger;  // use tbs::Logger internally

   log::StdoutLogger logger;
   logger.setLevel(log::Level::TraceMask);

   http::SettingsClient settings;
   settings
      .address("127.0.0.1")
      .port(8085)
      .tlsMode(true)
      .logVerbose(true)
      .connectionPoolSize(4)        // create max 4 connections
      .maxRequestsPerConnection(0); // disable

   http::SecureClientDefault client(std::move(settings), logger);

   // With a maximum of 4 connections, in async mode, 100 requests will
   // create 4 connections, each handling about 25 requests.

   // In async mode, shutdown() must be called,
   // otherwise the client will keep running.
   // It is safe to shutdown when no connections remain.
   // This can be checked in onConnectFailed() and onConnectionClosed() handlers.
   auto checkShutdown = [&]() {
      auto nConn = client.totalConnections();
      if( nConn == 0 )
      {
         logger.info("Shutting down Client");
         client.shutdown();
      }
   };

   // Setup handlers
   // ----------------------------------
   client.onConnectionError( [&](const http::ErrorData& error) {
      logger.error(error.message);
   });

   // check if we need to shutdown and log error
   client.onConnectFailed( [&](const std::string& message) {
      logger.error(message);
      checkShutdown();
   });

   // check if we need to shutdown
   client.onConnectionClosed([&](http::ConnectionId connId) {
      checkShutdown();
   });

   auto processResponse = [&](const http::ClientResponsePtr& response)
   {
      std::cout << "---------------------------------------------------" << std::endl;
      std::cout << "   Http Ver major  : " << response->majorVersion()   << std::endl;
      std::cout << "   Http Ver minor  : " << response->minorVersion()   << std::endl;
      std::cout << "   Status code     : " << response->statusCode()     << std::endl;
      std::cout << "   Status message  : " << response->statusMessage()  << std::endl;

      for ( size_t i = 0; i < response->headers().size(); i++)
      {
         auto f = response->headers().field(i);
         if ( f != nullptr )
            std::cout << "   Http header     : " <<  f->name() << " : " << f->value() << std::endl;
      }

      std::cout << "   Message body    : " << std::endl;
      std::cout << "   " << response->content() << std::endl;
      std::cout << "---------------------------------------------------" << std::endl;
   };

   // add default header for connection
   client.addHeader("Accept", "*/*");
   client.addHeader("User-Agent", "Tobasa Client");

   // Note: Do not mix async and sync request. use only one mode
   bool useSync = true;
   if (useSync)
   {
      // run 100 Sync requests

      std::string resource = "/api/version";
      for (int i=0;i<10;i++)
      {
         if (i>0)
            resource = "/api/version" + std::to_string(i);

         auto res = client.get(resource);
         if (res)
            processResponse(res);
      }

      auto res = client.post("/api/users/authenticate",
         R"-(
         {
            "userName": "admin",
            "password":"xxxxxxx"
         } )-"
      );

      if (res)
         processResponse(res);

      // client.shutdown(); // no need
   }
   else
   {
      // run 100 Async requests

      std::string resource = "/api/version";
      for (int i=0;i<10;i++)
      {
         if (i>0)
            resource = "/api/version" + std::to_string(i);
         
         client.get(resource, [&](const http::ClientResponsePtr& response) {
            processResponse(response);
         });
      }

      // in async request, client will run until shutdown() called
      client.run(2);
   }
}

int main(int argc, char* argv[])
{
   try
   {
      if (! tbs::DateTime::initTimezoneData())
         return 1;

      std::cout << "TOBASA HTTPS Client\n";
      runClient();
   }
   catch (std::exception& ex)
   {
      std::cout << ex.what() << "\n";
   }
   catch(...)
   {
      std::cout << "Exception occured";
   }

   return 0;
}