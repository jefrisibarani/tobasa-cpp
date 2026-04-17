#include <iostream>
#include <tobasa/config.h>
#include <tobasahttp/client/http_client.h>
#include <tobasaweb/settings_log.h>
#include <tobasaweb/multi_logger.h>

bool loadConfig()
{
   using namespace tbs;
   using namespace tbs::log;
   using namespace tbs::http;

   std::string _configFile;

   std::cout << "Setting up aplication configuration..." << "\n";
   try
   {
      if ( _configFile.length() == 0 )
         _configFile = "appsettings.json";

      // load configurations
      Config::get().load(_configFile);

      if (Config::get().valid())
      {
         auto logOption = Config::getOption<conf::Logging>("logging");
         // setup Tobasa Logger actual target
         Logger::setTarget(new log::MultiLogger(std::move(logOption))) ;

         return true;
      }
   }
   catch (Json::exception& e)
   {
      std::cerr << "Configuration exception: " << e.what() << "\n";
   }
   catch(const std::exception& e)
   {
      std::cerr << e.what() << '\n';
   }

   return false;
}

void runClient()
{
   using namespace tbs::log;
   using namespace tbs::http;

   TobasaLogger logger;  // use tbs::Logger internally

   SettingsClient settings;
   settings
      .address("localhost")
      .port(8085)
      .tlsMode(true)
      .logVerbose(true)
      .connectionPoolSize(4)        // create max 4 connections
      .maxRequestsPerConnection(0); // disable

   SecureClient client(std::move(settings), logger);

   auto checkShutdown = [&]() {
      auto nConn = client.totalConnections();
      if( nConn == 0 )
      {
         logger.info("Shutting down Client");
         client.shutdown();
      }
   };

   client.onConnectionError( [&](const ErrorData& error) {
      logger.error(error.message);
   });

   // check if we need to shutdown and log error
   client.onConnectFailed( [&](const std::string& message) {
      logger.error(message);
      checkShutdown();
   });

   // check if we need to shutdown
   client.onConnectionClosed([&](ConnectionId connId) {
      checkShutdown();
   });

   client.addHeader("Accept", "*/*");
   client.addHeader("Connection", "keep-alive");
   client.addHeader("User-Agent", "Tobasa Client");

   // run async get request
   client.get("/version", [&](const ClientResponsePtr& response)
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
   });

   // in async request, client will run until shutdown() called
   client.run();


   // reset Loggers' log target
   tbs::Logger::destroyInternalLogSink() ;
}

int main(int argc, char* argv[])
{
   try
   {
      if (! tbs::DateTime::initTimezoneData())
         return 1;

      loadConfig();
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
