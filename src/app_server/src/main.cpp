#include <iostream>
#include <tobasa/util.h>
#include <tobasa/path.h>
#include <tobasa/datetime.h>
#include <tobasa/config.h>
#include <tobasaweb/multipart_middleware.h>
#include <tobasaweb/webapp.h>

#include "database_service_factory_app.h"
#include "api_result.h"
#include "core/core_controller.h"
#include "core/core_admin_controller.h"
#include "core/api_core_controller.h"
#include "core/api_users_controller.h"

#include "middleware/cache_control.h"
#include "middleware/response_header_rule.h"
#include "middleware/request_identification.h"
#include "middleware/content_type.h"
#include "middleware/database_check.h"
#include "middleware/session.h"
#include "middleware/authorization.h"
#include "middleware/authentication.h"
#include "middleware/exception_handler.h"
#include "settings_header_rules.h"
#include "app_common.h"
#include "app_util.h"
#include "page.h"
#include "main_helper.h"
#include "app_resource.h"

#ifdef TOBASA_USE_TESTS_MODULE
   #include "test/test_controller.h"
   #include "test_ws/websocket_controller.h"
   #include "db_migrations/002_initial_base_module_test.h"
#endif

#ifdef TOBASA_USE_LIS_ENGINE
   #include "lis/lis_modul.h"
   #include "db_migrations/003_initial_base_module_lis.h"
#endif


struct ModuleConfig
{
   bool useLisEngine   = true;
   std::function<void()> startLisEngine = [](){};
   std::function<void()> stopLisEngine  = [](){};
};

int main(int argc, char* argv[])
{
   using namespace tbs;

   ModuleConfig moduleConfig;

   web::Page::releaseMode(false);

   if (! DateTime::initTimezoneData())
      return 1;


   std::cout << "[webapp] Current Working Dir: " << app::currentWorkingDir() << std::endl;
#if !defined(TOBASA_USE_STD_DATE) && defined(TOBASA_DATE_USE_IN_MEMORY_TZDB)
   std::cout << "[webapp] Built with in-memory time zone DB" << std::endl;
#endif
#ifdef TOBASA_BUILD_IN_MEMORY_RESOURCES
   std::cout << "[webapp] Built with in-memory resources" << std::endl;
#endif


   try
   {
      // Prepare web application object
      web::Webapp webapp;


      // -------------------------------------------------------
      // Configuration
      // -------------------------------------------------------
      std::string configFile = app::configDir() + path::SEPARATOR + "appsettings.json";
      auto embeddedConfig = app::Resource::get("config/appsettings.json", "config");
      if (! webapp.loadConfig(configFile, embeddedConfig) )
         return 1;

      // Main configuration
      auto webappOpt = Config::getOption<web::conf::Webapp>("webapp");



#ifdef TOBASA_BUILD_IN_MEMORY_RESOURCES
      // Force not to use in-memory resources
      web::conf::Webapp::useInMemoryResources = webappOpt.webService.useInMemoryResources;
#else
      web::conf::Webapp::useInMemoryResources = false;
#endif

      if (web::conf::Webapp::useInMemoryResources) {
         std::cout << "[webapp] Using in-memory resources" << std::endl;
      }


      // -------------------------------------------------------
      // Base database migration
      // -------------------------------------------------------
      {
         #ifdef TOBASA_USE_TESTS_MODULE
         webapp.migrationJob().add<dbm::InitialBaseModuleTest>();  // add test for base database
         #endif

         #ifdef TOBASA_USE_LIS_ENGINE
         webapp.migrationJob().add<dbm::InitialBaseModuleLIS>();  // add lis for base database
         #endif
      }


      // create and set custom db service for webapp. webapp will own dbservice
      auto dbService = std::make_shared<app::DbServiceFactoryApp>();
      dbService->addConnectorOption("MainAppDbOption", webappOpt.dbConnection);
      webapp.useDbService(dbService);

      // setup homepage
      auto homePage  = webappOpt.webService.homePage;
      homePage = homePage.empty() ? "/dashboard" : homePage;
      tbs::web::Page::homePage(homePage);

      // load http header rule configuration
      std::string headerRulefile = app::configDir() + path::SEPARATOR + "appsettings_header_rules.json";
      auto embeddedHeaderRule = app::Resource::get("config/appsettings_header_rules.json", "config");
      Config::addOption<web::conf::HttpResponseHeaderRule>("httpResponseHeaderRule", headerRulefile, embeddedHeaderRule);

      // Add content renderer for http::StatusResult  to replace default renderer: http::statusPageHtml()
      webapp.router()->addResultContentBuilder(
         [](std::shared_ptr<http::Result> result)
         {
            try
            {
               // app::renderStatusPage, use template to render status page
               return app::renderStatusPage(result);
            }
            catch(const std::exception&/*ex*/)
            {
               // Fallback: app::renderStatusPage may throw an exception due to the inability
               // to retrieve resources from disk (templates, files).
               // Instead, we use http::statusPageHtml, which generates HTTP content without using any disk files
               return http::statusPageHtml(result->httpStatus());
            }
         }
         , "http::StatusResult"
      );

      // Add a status page HTML renderer with a custom template to override the default renderer http::statusPageHtml()
      //webapp.serverStatusPageBuilder(
      //   [](std::shared_ptr<http::StatusPageData> data) {
      //      return app::renderStatusPage(data);
      //   });


      // -------------------------------------------------------
      // Middlewares
      // -------------------------------------------------------

      // ExceptionHandler Middleware
      webapp.addMiddleware(
         [](const http::HttpContext& context, const http::RequestHandler& next) {
            return web::exceptionHandlerMiddleware(context, next);
         } , "ExceptionHandler" );

      // Add middleware to perform a database connectivity check before processing http requests
      webapp.addMiddleware(
         [&webapp, &webappOpt, &dbService](const http::HttpContext& context, const http::RequestHandler& next) {
            return web::databaseCheckMiddleware(webapp, dbService, context, next);
         } , "DatabaseCheck" );

      // Multipart middleware to parse multipart body
      webapp.useMultipart(
         [&webappOpt](web::MultipartMiddlewareOption& option) {
            option.temporaryDir = webappOpt.httpServer.temporaryDir;
         } );

      // Add middleware to check client User-Agent, or any custom header related to our App
      webapp.addMiddleware(
         [](const http::HttpContext& context, const http::RequestHandler& next) {
            return web::responseHeaderRuleMiddleware(context, next);
         } , "ResponseHeaderRule" );

      // Add HTTP Response header middleware / CORS
      webapp.addMiddleware(
         [](const http::HttpContext& context, const http::RequestHandler& next) {
            return web::requestIdentificationMiddleware(context, next);
         } , "RequestIdentification" );

      // Add Cache-control middleware
      webapp.addMiddleware(
         [](const http::HttpContext& context, const http::RequestHandler& next) {
            return web::cacheControlMiddleware(context, next);
         } , "CacheControl" );

      // Add middleware to validate the content-type header before executing authentication and authorization processes
      webapp.addMiddleware(
         [](const http::HttpContext& context, const http::RequestHandler& next) {
            return web::contentTypeMiddleware(context,next);
         } , "ContentType" );

      // Session
      webapp.useSession(
         [&webappOpt](web::SessionMiddlewareOption& option) {
            web::builSessionMiddlewareOption(webappOpt, option);
         } );

      // Authentication
      webapp.useAuthentication(
         [&webappOpt](web::AuthenticationMiddlewareOption& option) {
            web::buildAuthenticationMiddlewareOption(webappOpt, option);
         } );

      // Authorization middleware last in the chain
      webapp.useAuthorization(
         [&webappOpt](web::AuthorizationMiddlewareOption& option) {
            web::builAuthorizationMiddlewareOption(webappOpt, option);
         } );


      // -------------------------------------------------------
      // Controllers
      // -------------------------------------------------------
      webapp.addController( web::makeController<app::CoreController>(dbService) );
      webapp.addController( web::makeController<app::ApiUsersController>(dbService) );
      webapp.addController( web::makeController<app::AdminController>(dbService) );
      webapp.addController( web::makeController<app::ApiCoreController>(dbService, webapp.agent()) );

#ifdef TOBASA_USE_TESTS_MODULE
      webapp.addController( web::makeController<test::TestController>() );
      webapp.addController( web::makeController<test::WebsocketController>() );
#endif


      // -------------------------------------------------------
      // LIS  Engine part
      // -------------------------------------------------------
#ifdef TOBASA_USE_LIS_ENGINE
      AppLisModule lisModule(moduleConfig.useLisEngine);
      lisModule.init(dbService, webapp, webappOpt);
      
      moduleConfig.startLisEngine = [&lisModule](){
         lisModule.startLisEngine();
      };
      moduleConfig.stopLisEngine = [&lisModule](){
         lisModule.stopLisEngine();
      };
#endif


      // -------------------------------------------------------
      // Default tls resources callback
      // -------------------------------------------------------
      webapp.defaultTlsAssetCallback( [](http::TlsAsset asset) 
      {
         if (asset==http::TlsAsset::cerificate_chain)
            return app::Resource::get("tls_asset/127.0.0.1.crt", "tls_asset");
         if (asset==http::TlsAsset::private_key)
            return app::Resource::get("tls_asset/127.0.0.1.key", "tls_asset");
         if (asset==http::TlsAsset::tmp_dh)
            return app::Resource::get("tls_asset/dh2048.pem", "tls_asset");

         return nonstd::span<const unsigned char>();
      });


      // -------------------------------------------------------
      // APP's onStart() and onStop()
      // -------------------------------------------------------
      webapp.onStart( [&moduleConfig]() {
         moduleConfig.startLisEngine();
      });

      webapp.onStop( [&moduleConfig]() {
         moduleConfig.stopLisEngine();
      });


      // Start the app
      webapp.start();

   }
   catch (const std::exception& ex)
   {
      std::cerr << ex.what() << "\n";
   }
   catch(...)
   {
      std::cerr << "[webapp] Exception occured\n";
   }

   std::cerr << "[webapp] App destroyed\n";
   return 0;
}