#include <tobasa/datetime.h>
#include <mutex>
#include "tobasahttp/client/http_client.h"

void runClient(int totalRequest, bool asyncMode)
{
   using namespace tbs;

   log::StdoutLogger logger;
   logger.setLevel(log::Level::TraceMask);

   http::SettingsClient settings;
   settings
      .address("127.0.0.1")
      .port(8085)
      .tlsMode(true)
      .logVerbose(true)
      .verifyPeer(false)            // TLS is encrypted, but the server certificate is not checked
      .connectionPoolSize(4)        // create max 4 connections
      .maxRequestsPerConnection(0); // disable

   http::SecureClientDefault client(std::move(settings), logger);

   // With a maximum of 4 connections, in async mode, 100 requests will
   // create 4 connections, each handling about 25 requests.

   // shutdown() drains submitted requests before run() returns.

   std::mutex outputMutex;
   auto processResponse = [&outputMutex](const http::ClientResponsePtr& response)
   {
      // just to protect the whole output block, so it prints as one block
      std::lock_guard<std::mutex> lock(outputMutex);

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
   if (!asyncMode)
   {
      // run Sync request
      std::string resource = "/api/version";
      for (int i=0; i<totalRequest; i++)
      {
         if (i>0)
            resource = "/api/version" + std::to_string(i);

         auto res = client.get(resource);
         if (res.responseReceived())
            processResponse(res.response);
         else
            logger.error("Request failed: {}", res.error.message);
      }

      auto res = client.post("/api/users/authenticate",
         R"-(
         {
            "userName": "admin",
            "password":"xxxxxxx"
         } )-"
      );

      if (res.responseReceived())
         processResponse(res.response);
      else
         logger.error("Request failed: {}", res.error.message);

      // client.shutdown(); // no need
   }
   else
   {
      // run Async request
      std::string resource = "/api/version";
      for (int i=0; i<totalRequest; i++)
      {
         if (i>0)
            resource = "/api/version" + std::to_string(i);
         
         client.get(resource, [&](const http::ClientResult& result) {
            if (result.responseReceived())
               processResponse(result.response);
            else
               logger.error("Request failed: {}", result.error.message);
         });
      }

      client.shutdown();
      client.run(2);
   }
}

int main(int argc, char* argv[])
{
   try
   {
      int totalRequest = 100;
      bool asyncMode = false;

      if (! tbs::DateTime::initTimezoneData())
         return 1;

      std::cout << "TOBASA HTTPS Client\n";
      runClient(totalRequest,asyncMode);
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