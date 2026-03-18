#pragma once

#include <tobasa/datetime.h>
#include <tobasaweb/controller_base.h>
#include <tobasaweb/router.h>
#include <tobasalis/lis/engine.h>
#include <tobasalis/lis/settings.h>
#include <asio/thread_pool.hpp>
#include "../database_service_factory_app.h"
#include "lis/lis_db_service_hl7.h"
#include "lis/lis_db_service_lis2a.h"
#include "../page.h"
#include "../database_service_factory_app.h"
#include "lis_util.h"
#include "engine_stats.h"

namespace tbs {
namespace lis {

class DbServiceFactory;
using ThreadPool = asio::thread_pool;

class ApiLisController : public web::ControllerBase
{
private:
   ApiLisController( const ApiLisController & ) = delete;
   ApiLisController( ApiLisController && ) = delete;

   app::DbServicePtr           _dbService  {nullptr};
   std::shared_ptr<LisEngine>  _lisEngine  = nullptr;
   // Application database service thread pool
   std::shared_ptr<ThreadPool> _dbThreadPool;

   conf::Engine                _option;
   sql::conf::ConnectorOption  _dbOption;
   web::dom::MenuGroup         _menuGroup;
   bool _hl7ProtocolMode       = false;
   
   std::shared_ptr<DateTime>   _startedTime;
   std::shared_ptr<DateTime>   _stoppedTime;
   
   // LIS Engine database service wrapper
   std::shared_ptr<DbServiceFactory> _dbServiceFactory;

public:
   explicit ApiLisController(app::DbServicePtr dbService,
      std::shared_ptr<LisEngine> lisEngine, 
      std::shared_ptr<ThreadPool> dbThreadPool);
   ~ApiLisController();

   virtual void bindHandler();

   //! Handle GET request to /lis/server_status
   http::ResultPtr onLisServerStatus(const web::RouteArgument& arg);

   //! Handle GET request to /api/lis/server_status
   http::ResultPtr onApiServerStatus(const web::RouteArgument& arg);

   //! Handle POST request to /api/lis/start_engine
   http::ResultPtr onApiStartEngine(const web::RouteArgument& arg);

   //! Handle POST request to /api/lis/stop_engine
   http::ResultPtr onApiStopEngine(const web::RouteArgument& arg);

   //! Handle POST request to /api/lis/send_hl7_message
   http::ResultPtr onApiSendHL7Message(const web::RouteArgument& arg);

   //! Handle POST request to /api/lis/send_lis_message
   http::ResultPtr onApiSendLisMessage(const web::RouteArgument& arg);

   //! Handle POST request to /api/lis/parse_and_send_message
   http::ResultPtr onApiTestParseAndSendMessage(const web::RouteArgument& arg);

   //! Handle GET request to /api/lis/lis2a_result_list/{header_id}
   http::ResultPtr onApiLis2aResultList(const web::RouteArgument& arg);

   //! Handle GET request to /api/lis/hl7_obxlist/{obrid}/{patientid}
   http::ResultPtr onApiHl7ObxList(const web::RouteArgument& arg);

   //! Handle GET request to /lis/testdev_lis1a
   http::ResultPtr onLisDeviceTestDevLIS1A(const web::RouteArgument& arg);
   
   //! Handle GET request to /lis/testdev_hl7
   http::ResultPtr onLisDeviceTestDevHL7(const web::RouteArgument& arg);

private:


   std::shared_ptr<lis::LisEngine> lisEngine();   

   void hndlrLisEngineStarted(const std::string& arg);
   void hndlrLisEngineStopped(const std::string& arg);
   void hndlrLisConnectionCreated(const std::string& arg);
   void hndlrLisConnected(const std::string& arg);
   void hndlrLisConnectFailed(const std::string& arg);
   void hndlrLisSendProgress(double val);
   void hndlrLisRecordReady(std::shared_ptr<lis::Record>& record);
   void hndlrLisMessageReady(std::shared_ptr<lis::Message>& message);
   void hndlrLisMessageToStorage(std::shared_ptr<lis::Message>& message);
   // should call or run async function
   void hndlrLisCommunicationIdle(const std::string& arg);

   void putMessageToDatabase(const std::shared_ptr<lis::Message>& message);
   void updateEngineStatus();

   bool isLIS2A2ProtocolMode();
   // HL7 specific
   bool isHL7ProtocolMode();
   void hndlrLisMessageReady_HL7(std::shared_ptr<tbs::lis::Message>& message);

   // DevTest_LIS1A specific
   void hndlrLisMessageReady_DevTest_LIS1A(std::shared_ptr<tbs::lis::Message>& message, EngineState& engineState);
   void hndlrLisCommunicationIdle_DevTest_LIS1A();
   std::shared_ptr<lis::Message> prepareAckMessageToLisManager(std::shared_ptr<lis::Message>& message);
   std::shared_ptr<lis::Message> prepareOrderMessageToAnalyzer(std::shared_ptr<lis::Message>& message);
};

} // namespace lis
} // namespace tbs
