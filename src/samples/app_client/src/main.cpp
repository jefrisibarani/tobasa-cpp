#include <iostream>
#include <tobasa/config.h>
#include <tobasahttp/client/http_client.h>
#include <tobasaweb/settings_log.h>
#include <tobasaweb/settings_http_client.h>
#include <tobasaweb/multi_logger.h>

bool loadConfig()
{
   using namespace tbs;
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
         auto logOption = Config::getOption<log::conf::Logging>("logging");
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
   using namespace tbs;
   using namespace tbs::http;

   log::TobasaLogger logger;  // use tbs::Logger internally

   auto config = Config::getOption<http::conf::Client>("client");

   auto caFilePath = path::executableDir() + path::SEPARATOR + config.caVerificationFile;

   SettingsClient settings;
   settings
      .address(config.address)
      .port(config.port)
      .tlsMode(config.tlsMode)
      .verifyPeer(config.verifyPeer)
      .caVerificationFile(caFilePath)
      .logVerbose(config.logVerbose)
      .connectionPoolSize(config.connectionPoolSize)
      .maxRequestsPerConnection(config.maxRequestsPerConnection); // set 0 to disable

   SecureClient client(std::move(settings), logger);

   client.addHeader("Accept", "*/*");
   client.addHeader("Connection", "keep-alive");
   client.addHeader("User-Agent", "Tobasa Client");

   // run async get request
   client.get("/api/version", [&](const ClientResult& result)
   {
      if (!result.responseReceived())
      {
         logger.error("Request failed: {}", result.error.message);
         return;
      }

      const auto& response = result.response;
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

   client.shutdown();
   client.run();
   
   // Clear the logger before the program exits, while spdlog is still active.
   Logger::destroyInternalLogSink();
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
