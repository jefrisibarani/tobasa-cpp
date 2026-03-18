#pragma once

#include <thread>
#include <tobasaweb/webapp.h>
#include <tobasalis/lis/engine.h>
#include "../database_service_factory_app.h"

namespace tbs {

using LisThreadPool = asio::thread_pool;


class AppLisModule
{
public:
   ~AppLisModule();

   AppLisModule(bool& useModule);
   AppLisModule(const AppLisModule&) = default;

   void init(app::DbServicePtr dbService, web::Webapp& app, web::conf::Webapp& webappOpt);

   // The lifetime of `lisIoContext` must be longer than that of `lisThread`
   // to ensure that `lisIoContext` remains valid throughout the execution of `lisThread`.
   asio::io_context lisIoContext;
   std::shared_ptr<std::thread> lisThread      = nullptr;  // LIS Engine thread
   std::shared_ptr<LisThreadPool> lisdbTPool   = nullptr;  // LIS Db thread pool
   std::shared_ptr<lis::LisEngine> lisEngine   = nullptr;  // The LIS engine instance

   std::exception_ptr exceptionCaught;
   lis::conf::Engine lisOption;

   void startLisEngine();
   void stopLisEngine();

   bool enabled = false;
};


} // namespace tbs