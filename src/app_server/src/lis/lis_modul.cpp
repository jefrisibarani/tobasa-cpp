#include <tobasa/config.h>
#include "../app_common.h"
#include "../app_util.h"
#include "../app_resource.h"
#include "api_lis_controller.h"
#include "lis_modul.h"

#include <tobasaweb/db_migration.h>
#include "db_migrations/001_initial_module_lis.h"

namespace tbs {

AppLisModule::~AppLisModule() {}

AppLisModule::AppLisModule(bool& useModule)
{
   try
   {
      if (useModule)
      {
         std::string lisConfigFile = app::configDir() + path::SEPARATOR + "appsettings_lis.json";
         auto embeddedLisOpt = app::Resource::get("config/appsettings_lis.json", "config");
         Config::addOption<lis::conf::Engine>("lisEngine", lisConfigFile, embeddedLisOpt);
         lisOption = Config::getOption<lis::conf::Engine>("lisEngine");
         if (lisOption.autoStart)
            enabled = true;
         else
            Logger::logW("[app] LIS module is not auto start");
      }
   }
   catch (const std::exception& ex)
   {
      enabled = false;
      useModule = false;
      Logger::logE("[app] -------------------------------------------------------");
      Logger::logE("[app] LIS MODULE IS DISABLED");
      Logger::logE("[app] Failed loading LIS Module configuration: {}", ex.what());
      Logger::logE("[app] -------------------------------------------------------\n");
   }
}

void AppLisModule::init(app::DbServicePtr dbService, web::Webapp& app, web::conf::Webapp& webappOpt)
{
   if (!enabled)
      return;

   try
   {
      // Dedicated thread pool for lis database
      if (lisOption.result.dbThreadPoolSize < 1 )
         lisOption.result.dbThreadPoolSize = 2;
      else
      {
         auto hwc = static_cast<int>(std::thread::hardware_concurrency());
         if (lisOption.result.dbThreadPoolSize > hwc) 
            lisOption.result.dbThreadPoolSize = 2;
      }

      lisdbTPool = std::make_shared<LisThreadPool>(lisOption.result.dbThreadPoolSize);
      Logger::logT("[app] LIS DB Thread Pool created. Pool size: {} ", lisOption.result.dbThreadPoolSize);

      if (lisOption.useDedicatedRunningThread)
      {
         lisEngine = std::make_shared<lis::LisEngine>( lisIoContext, lisOption);
         app.addController( web::makeController<lis::ApiLisController>(dbService, lisEngine, lisdbTPool) );
      }
      else
      {
         // Use web service io_context

         // Note: ioPoolSize value must >= 2 (in appsettings.json)
         // otherwise LisEngine stuck waiting for <ACK>
         if (webappOpt.httpServer.ioPoolSize < 2 )
         {
            Logger::logE("[app] ");
            Logger::logE("[app] With current setting; LisEngine will use web service io_context,");
            Logger::logE("[app] but will not work with thread pool size < 2 ");
            Logger::logE("[app] Update appsettings.json, set httpServer.ioPoolSize >= 2 ");
            Logger::logE("[app] Or in appsettings_lis.json, use useDedicatedRunningThread=true ");
            Logger::logE("[app] For now, we disable LIS Module. ");
            Logger::logE("[app] ");
            
            throw std::runtime_error("Invalid thread pool size");
         }
         else
         {
            lisEngine = std::make_shared<lis::LisEngine>( app.webService()->ioContext(), lisOption);
            app.addController( web::makeController<lis::ApiLisController>(dbService, lisEngine, lisdbTPool) );
         }
      }
   }
   catch (const std::exception& ex)
   {
      enabled = false;
      Logger::logE("[app] -------------------------------------------------------");
      Logger::logE("[app] LIS MODULE IS DISABLED");
      Logger::logE("[app] Error intializing LIS Module: {}", ex.what());
      Logger::logE("[app] -------------------------------------------------------\n");
   }
}

void initLisDatabase()
{
   auto connOpt = lis::getDbConnectorOption();

   dbm::MigrationJob job;
   job.add<dbm::InitialModuleLIS>();
   dbm::runDbMigration(&connOpt, job);
}

// LIS engine starter
void AppLisModule::startLisEngine()
{
   if (!enabled)
      return;

   auto threadId0 = util::threadId(std::this_thread::get_id());

   if (lisOption.useDedicatedRunningThread)
   {
      // run LIS Engine IO Context
      lisThread = std::make_shared<std::thread>(
         [&, threadId0]
         {
            try
            {
               if (lisOption.autoStart)
               {
                  Logger::logI("[app] Initializing LIS engine, from thread id {}", threadId0 );
                  initLisDatabase();
                  lisEngine->start();
               }
               auto work{ asio::make_work_guard(lisIoContext) };
               lisIoContext.run();
            }
            catch (...)
            {
               exceptionCaught = std::current_exception();

               lisEngine->stop();
               lisIoContext.stop();

               if (lisThread && lisThread->joinable())
                  lisThread->join();

               if (lisdbTPool)
                  lisdbTPool->join();
            }
         });

      auto tid = util::threadId(lisThread->get_id());
      Logger::logT("[app] LIS engine dedicated thread id {} created, callled from thread id {}", tid, threadId0 );

      // If an error was detected it should be propagated.
      if (exceptionCaught)
         std::rethrow_exception( exceptionCaught );
   }
   else
   {
      if (lisOption.autoStart)
      {
         Logger::logI("[app] Initializing LIS engine, from thread id {}", threadId0 );
         initLisDatabase();
         lisEngine->start();
         lisIoContext.run();
      }
   }
}

// LIS engine stopper
void AppLisModule::stopLisEngine()
{
   if (!enabled)
      return;

   if (lisOption.useDedicatedRunningThread)
   {
      lisEngine->stop();
      lisIoContext.stop();

      if (lisThread && lisThread->joinable())
         lisThread->join();

      if (lisdbTPool)
         lisdbTPool->join();
   }
   else
   {
      lisEngine->stop();

      if (lisdbTPool)
         lisdbTPool->join();
   }
}



} // namespace tbs