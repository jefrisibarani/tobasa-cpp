#include <future>
#include <tobasa/logger.h>
#include <tobasa/json.h>
#include <tobasaweb/json_result.h>
#include <tobasaweb/credential_info.h>
#include <tobasaweb/alert.h>
#include <tobasaweb/session.h>
#include <tobasaweb/util.h>
#include <tobasa/config.h>
#include <tobasa/path.h>
#include <tobasa/util.h>
#include <tobasalis/lis/common.h>
#include <tobasalis/lis2a/message.h>
#include <tobasalis/bci/message.h>
#include "../inja.hpp"
#include "../api_result.h"
#include "../app_util.h"
#include "../page.h"
#include "lis_db_service_factory.h"
#include "build_info.h"
#include "api_lis_controller.h"


namespace tbs {
namespace lis {

using namespace http;

std::mutex engineStateMutex;
static EngineState engineState;

static bool lisDatabaseInitialized = false;

ApiLisController::ApiLisController(app::DbServicePtr dbService,
      std::shared_ptr<LisEngine> lisEngine, 
      std::shared_ptr<ThreadPool> dbThreadPool)
   : web::ControllerBase()
   , _dbService {dbService}
   , _lisEngine {lisEngine}
   , _dbThreadPool {dbThreadPool}
{
   _option = Config::getOption<conf::Engine>("lisEngine");
   // Get lisEngine database connection option
   _dbOption = lis::getDbConnectorOption();

   _menuGroup.icon      = "fas fa-toolbox";
   _menuGroup.groupName = "lis";
   _menuGroup.caption   = "LIS";
   _menuGroup.menuList.emplace_back("Server Status",      "lis/server_status",  "fas fa-bars");

   if (_option.activeInstrument == lis::DEV_DEFAULT_LIS1A || _option.activeInstrument == lis::DEV_TEST_LIS1A)
      _menuGroup.menuList.emplace_back("Dev Test LIS1A",  "lis/testdev_lis1a",  "fas fa-notes-medical");

   if (_option.activeInstrument == lis::DEV_DEFAULT_HL7 || _option.activeInstrument == lis::DEV_TEST_HL7)
      _menuGroup.menuList.emplace_back("Dev Test HL7",    "lis/testdev_hl7",    "fas fa-notes-medical");

   web::Page::menuGroupList.emplace_back(_menuGroup);

   _hl7ProtocolMode = isHL7ProtocolMode();

   _dbServiceFactory = std::make_shared<DbServiceFactory>(_dbOption, _option.activeInstrument);

   engineState.appThreadId = util::threadId(std::this_thread::get_id());
}

ApiLisController::~ApiLisController()
{}

void ApiLisController::bindHandler()
{
   using namespace std::placeholders;
   auto self(this);

   //! Handle GET request to /lis/server_status
   router()->httpGet("/lis/server_status",
      std::bind(&ApiLisController::onLisServerStatus, self, _1),            AuthScheme::COOKIE);
   //! Handle GET request to /api/lis/server_status
   router()->httpGet("/api/lis/server_status",
      std::bind(&ApiLisController::onApiServerStatus, self, _1),            AuthScheme::BEARER);
   //! Handle request to POST /api/lis/start_engine
   router()->httpPost("/api/lis/start_engine",
      std::bind(&ApiLisController::onApiStartEngine, self, _1),             AuthScheme::BEARER);
   //! Handle request to POST  /api/lis/stop_engine
   router()->httpPost("/api/lis/stop_engine",
      std::bind(&ApiLisController::onApiStopEngine, self, _1),              AuthScheme::BEARER);

   //! Handle request to /api/lis/send_hl7_message
   router()->httpPost("/api/lis/send_hl7_message",
      std::bind(&ApiLisController::onApiSendHL7Message, self, _1),          AuthScheme::BEARER);

   //! Handle request to POST /api/lis/send_lis_message
   router()->httpPost("/api/lis/send_lis_message",
      std::bind(&ApiLisController::onApiSendLisMessage, self, _1),          AuthScheme::BEARER);
   //! Handle request to POST  /api/lis/parse_and_send_message
   router()->httpPost("/api/lis/parse_and_send_message",
      std::bind(&ApiLisController::onApiTestParseAndSendMessage, self, _1), AuthScheme::BEARER);

   //! Handle GET request to /api/lis/lis2a_result_list/{header_id}
   router()->httpGet("/api/lis/lis2a_result_list/{header_id}",
      std::bind(&ApiLisController::onApiLis2aResultList, self, _1),         AuthScheme::BEARER);


   //! Handle GET request to /api/lis/hl7_obxlist/{obrid}/{patientid}
   router()->httpGet("/api/lis/hl7_obxlist/{obrid}/{patientid}",
      std::bind(&ApiLisController::onApiHl7ObxList, self, _1),              AuthScheme::BEARER);

   //! Handle GET request to /lis/testdev_lis1a?startdate={startdate}&enddate={enddate}&offset={offset}&limit={limit}&filter={filter}
   if ( _option.activeInstrument == lis::DEV_DEFAULT_LIS1A || _option.activeInstrument == lis::DEV_TEST_LIS1A)
   {
      router()->httpGet("/lis/testdev_lis1a",
         std::bind(&ApiLisController::onLisDeviceTestDevLIS1A, self, _1),          AuthScheme::COOKIE);
   }

   //! Handle GET request to /lis/testdev_hl7?startdate={startdate}&enddate={enddate}&offset={offset}&limit={limit}&filter={filter}
   if ( _option.activeInstrument == lis::DEV_DEFAULT_HL7 || _option.activeInstrument == lis::DEV_TEST_HL7)
   {
      router()->httpGet("/lis/testdev_hl7",
         std::bind(&ApiLisController::onLisDeviceTestDevHL7, self, _1),     AuthScheme::COOKIE);
   }

   if ( lisEngine() != nullptr )
   {
      // bind LisEngines's events
      lisEngine()->onEngineStarted     = std::bind(&ApiLisController::hndlrLisEngineStarted,     self, _1);
      lisEngine()->onEngineStopped     = std::bind(&ApiLisController::hndlrLisEngineStopped,     self, _1);
      lisEngine()->onConnectionCreated = std::bind(&ApiLisController::hndlrLisConnectionCreated, self, _1);
      lisEngine()->onConnected         = std::bind(&ApiLisController::hndlrLisConnected,         self, _1);
      lisEngine()->onConnectFailed     = std::bind(&ApiLisController::hndlrLisConnectFailed,     self, _1);
      lisEngine()->onSendProgress      = std::bind(&ApiLisController::hndlrLisSendProgress,      self, _1);
      lisEngine()->onRecordReady       = std::bind(&ApiLisController::hndlrLisRecordReady,       self, _1);
      lisEngine()->onMessageReady      = std::bind(&ApiLisController::hndlrLisMessageReady,      self, _1);
      lisEngine()->onMessageToStorage  = std::bind(&ApiLisController::hndlrLisMessageToStorage,  self, _1);
      lisEngine()->onCommunicationIdle = std::bind(&ApiLisController::hndlrLisCommunicationIdle, self, _1);
   }
}

std::shared_ptr<lis::LisEngine>
ApiLisController::lisEngine() { return _lisEngine; }

//! Handle GET request to /api/lis/server_status
http::ResultPtr ApiLisController::onApiServerStatus(const web::RouteArgument& arg)
{
   if ( ! lisEngine() )
      return web::okResult("LIS Engine is disabled", 202);

   updateEngineStatus();

   Json resp = Json::array();
   resp.emplace_back( Json::array_t{0, "LIS Engine thread id", engineState.threadId } );
   resp.emplace_back( Json::array_t{1, "App thread id",        engineState.appThreadId } );
   resp.emplace_back( Json::array_t{2, "Host ID",              engineState.lisHostId } );
   resp.emplace_back( Json::array_t{3, "Host Provider",        engineState.lisHostProvider } );
   resp.emplace_back( Json::array_t{4, "Role",                 engineState.lisRole } );
   resp.emplace_back( Json::array_t{5, "Startup mode",         engineState.lisStartupMode } );
   resp.emplace_back( Json::array_t{6, "Instrument",           engineState.lisInstrument } );

   if ( lisEngine()->isStarted() )
   {
      std::string runningState = engineState.lisRunningState + ", since " + _startedTime->isoDateTimeString();
      resp.emplace_back( Json::array_t{7, "Status", runningState } );
      resp.emplace_back( Json::array_t{8, "Uptime", engineState.lisUptime } );
   }
   else
   {
      std::string runningState = engineState.lisRunningState;
      if (_stoppedTime)
         runningState += ", since " + _stoppedTime->isoDateTimeString();
      resp.emplace_back( Json::array_t{7, "Status", runningState } );
      resp.emplace_back( Json::array_t{8, "Downtime", engineState.lisDownTime } );
   }

   resp.emplace_back( Json::array_t{9,  "LIS Mode",                         engineState.lisTcpMode } );
   resp.emplace_back( Json::array_t{10, "LIS Connection State",             engineState.lisTCPConnState } );
   resp.emplace_back( Json::array_t{11, "LIS Link State",                   engineState.lisLinkState } );
   resp.emplace_back( Json::array_t{12, "Total Message Received",           engineState.totalMessageReceived } );
   resp.emplace_back( Json::array_t{13, "Total Request Message Received",   engineState.totalRequestMessageReceived } );
   resp.emplace_back( Json::array_t{14, "Total Order Message ACK Received", engineState.totalOrderMessageReceivedACK } );
   resp.emplace_back( Json::array_t{15, "LIS database",        engineState.lisDbInfo } );
   resp.emplace_back( Json::array_t{16, "App name",            engineState.appNameVersion } );
   resp.emplace_back( Json::array_t{17, "App build date",      engineState.appBuildDate } );
   resp.emplace_back( Json::array_t{18, "App compiler",        engineState.appcompilerInfo } );
   resp.emplace_back( Json::array_t{19, "App database",        engineState.appDbInfo } );
   resp.emplace_back( Json::array_t{20, "Errors",              engineState.lisErrors } );

   return web::object(resp);
}

//! Handle request to /api/lis/start_engine
http::ResultPtr ApiLisController::onApiStartEngine(const web::RouteArgument& arg)
{
   if ( ! lisEngine())
      return web::okResult("LIS Engine is disabled", 202);

   if ( lisEngine()->isStarted() )
      return web::okResult("LIS Engine already started", 200);
   else
   {
      if ( lisEngine()->start() )
         return web::okResult("LIS Engine successfully started", 200);
      else
         return web::okResult("LIS Engine failed to start", 202);
   }
}

//! Handle request to /api/lis/stop_engine
http::ResultPtr ApiLisController::onApiStopEngine(const web::RouteArgument& arg)
{
   if ( ! lisEngine())
      return web::okResult("LIS Engine is disabled", 202);

   if ( ! lisEngine()->isStarted() )
      return web::okResult("LIS Engine already stopped", 200);
   else
   {
      if ( lisEngine()->stop() )
         return web::okResult("LIS Engine successfully stopped", 200);
      else
         return web::okResult("LIS Engine failed to stop", 202);
   }
}

//! Handle POST request to /api/lis/send_hl7_message
http::ResultPtr ApiLisController::onApiSendHL7Message(const web::RouteArgument& arg)
{
   if (! isHL7ProtocolMode()) {
      return web::okResult("Only work with HL7 Protocol mode Instruments", 202);
   }

   auto httpCtx = arg.httpContext();
   if (!util::startsWith(httpCtx->request()->contentType(), "application/json"))
      return web::badRequest("invalid content type");

   if ( ! lisEngine())
      return web::okResult("LIS Engine is disabled", 202);

   if ( ! lisEngine()->isStarted() )
      return web::okResult("LIS Engine is not running", 202);
   else
   {
      auto body    = httpCtx->request()->content();
      auto jsonDto = Json::parse(body);
      std::string message   = jsonDto["message"];
      std::string devType   = jsonDto["deviceType"];
      std::string endOfLine = jsonDto["endOfLine"];

      if (!lisEngine()->isConnected())
          return web::okResult("LIS Engine is not connected to the peer device", 202);

      lisEngine()->sendMessage(message, endOfLine);

      return web::okResult();
   }
}

//! Handle POST request to /api/lis/send_lis_message
http::ResultPtr ApiLisController::onApiSendLisMessage(const web::RouteArgument& arg)
{
   if (!isLIS2A2ProtocolMode()) {
      return web::okResult("Only work with LIS2A-2 Protocol mode Instruments", 202);
   }

   auto httpCtx = arg.httpContext();
   if (!util::startsWith(httpCtx->request()->contentType(), "application/json"))
      return web::badRequest("invalid content type");

   if ( ! lisEngine())
      return web::okResult("LIS Engine is disabled", 202);

   if ( ! lisEngine()->isStarted() )
      return web::okResult("LIS Engine is not running", 202);
   else
   {
      auto body    = httpCtx->request()->content();
      auto jsonDto = Json::parse(body);
      std::string message   = jsonDto["message"];
      std::string devType   = jsonDto["deviceType"];
      std::string endOfLine = jsonDto["endOfLine"];

      lisEngine()->sendMessage(message, endOfLine);

      return web::okResult();
   }
}

//! Handle POST request to /api/lis/parse_and_send_message
http::ResultPtr ApiLisController::onApiTestParseAndSendMessage(const web::RouteArgument& arg)
{
   if (! isLIS2A2ProtocolMode()) {
      return web::okResult("Only work with LIS2A-2 Protocol mode Instruments", 202);
   }

   auto httpCtx = arg.httpContext();
   if (!util::startsWith(httpCtx->request()->contentType(), "application/json"))
      return web::badRequest("invalid content type");

   if ( ! lisEngine())
      return web::okResult("LIS Engine is disabled", 202);

   if ( ! lisEngine()->isStarted() )
      return web::okResult("LIS Engine is not running", 202);
   else
   {
      auto body    = httpCtx->request()->content();
      auto jsonDto = Json::parse(body);
      std::string message   = jsonDto["message"];
      std::string devType   = jsonDto["deviceType"];
      std::string endOfLine = jsonDto["endOfLine"];

      lisEngine()->parseAndSendMessages(message, endOfLine);

      return web::okResult();
   }
}

//! Handle GET request to /lis/server_status
http::ResultPtr ApiLisController::onLisServerStatus(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();

   updateEngineStatus();

   auto session     = web::Session::get(httpCtx->sessionId());
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );

   auto page = std::make_shared<web::Page>(httpCtx);
   page->data("pageTitle",     "LIS Engine - Tobasa Web Service");
   page->data("pageBodyClass", "");
   page->data("sessExpNotice", 60); // seconds
   page->data("sessExpTime",   authResult.expirationTime);

   return page->show("lis/server_status.tpl", "appview_lis");
}

//! Handle GET request to /api/lis/lis2a_result_list/{header_id}
http::ResultPtr ApiLisController::onApiLis2aResultList(const web::RouteArgument& arg)
{
   if (! isLIS2A2ProtocolMode() ) {
      return web::okResult("Only work with LIS2A-2 Protocol mode Instruments", 202);
   }

   auto httpCtx = arg.httpContext();
   auto hid     = arg.get("header_id");
   if (hid)
   {
      if ( !util::isNumber(hid.value()) )
         return web::badParameter("Invalid value for parameter header id");

      if (std::stol(hid.value()) < 0)
         return web::badParameter("Invalid parameter header id");

      auto repo = _dbServiceFactory->lis2aService( _option.getDbDataWithNewConnection);
      auto list = repo->getLIS2AResultList(_option.activeInstrument, std::stol(hid.value())  );
      return web::object(list);
   }

   return web::badParameter("Invalid value for parameter header id");
}

//! Handle GET request to /api/lis/hl7_obxlist/{obrid}/{patientid}
http::ResultPtr ApiLisController::onApiHl7ObxList(const web::RouteArgument& arg)
{
   if (!isHL7ProtocolMode()) {
      return web::okResult("Only work with HL7 Protocol mode Instruments", 202);
   }

   auto httpCtx = arg.httpContext();
   auto id      = arg.get("obrid");
   auto pid     = arg.get("patientid");
   if (id && pid)
   {
      if ( !util::isNumber(id.value()) )
         return web::badParameter("Invalid value for parameter id");

      if ( !util::isNumber(pid.value()) )
         return web::badParameter("Invalid value for parameter patient id");

      if (std::stol(id.value()) <= 0)
         return web::badParameter("Invalid parameter id");

      if (std::stol(pid.value()) <= 0)
         return web::badParameter("Invalid parameter patient id");

      auto repo = _dbServiceFactory->lisHL7Service( _option.getDbDataWithNewConnection);
      auto obxList = repo->getHL7ObxList(_option.activeInstrument, std::stol(id.value()), std::stol(pid.value())  );
      return web::object(obxList);
   }

   return web::badParameter("Invalid value for parameter obr or pid id");
}

//! Handle GET request to /lis/testdev_lis1a?startdate={startdate}&enddate={enddate}&offset={offset}&limit={limit}&filter={filter}
http::ResultPtr ApiLisController::onLisDeviceTestDevLIS1A(const web::RouteArgument& arg)
{
   auto httpCtx     = arg.httpContext();
   auto alert       = web::Alert::create(httpCtx->sessionId());
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   auto repo        = _dbServiceFactory->lis2aService( _option.getDbDataWithNewConnection);

   std::string queryError, startDate, endDate;
   auto queryOption = util::getSqlQueryOption(arg, startDate, endDate, queryError);
   auto lisorders = repo->getLIS2AEntryList(lis::DEV_TEST_LIS1A, queryOption);
   if (!queryError.empty())
      alert->error(queryError, web::Alert::LOC_TOAST);

   web::dom::DatatableOpt datatableopt;
   auto page = std::make_shared<web::Page>(httpCtx);
   page->data("pageMenu",         _menuGroup);
   page->data("pageTitle",        "LIS Test Device LIS1A");
   page->data("pageLang",         "en");
   page->data("pageBodyClass",    "bg-primary");
   page->data("pageId",           "id_lis_testdev_lis1a");
   page->data("pageContentTitle", "LIS Test Device LIS1A");
   page->data("sessExpNotice",    60);
   page->data("sessExpTime",      authResult.expirationTime);
   page->data("pageBreadcrumb",   "LIS / Test Device");
   page->data("datatableId",      "id_table_lisorders");
   page->data("datatable",         datatableopt);
   page->data("dataStartDate",     startDate);
   page->data("dataEndDate",       endDate);
   page->data()["identity"]["userName"]  = authResult.identity.pUser->userName;
   page->data("lisorders", lisorders);

   return page->show("lis/testdev_lis1a.tpl", "appview_lis");
}

 //! Handle GET request to /lis/testdev_hl7?startdate={startdate}&enddate={enddate}&offset={offset}&limit={limit}&filter={filter}
http::ResultPtr ApiLisController::onLisDeviceTestDevHL7(const web::RouteArgument& arg)
{
   auto httpCtx     = arg.httpContext();
   auto alert       = web::Alert::create(httpCtx->sessionId());
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   auto repo        = _dbServiceFactory->lisHL7Service( _option.getDbDataWithNewConnection);

   std::string queryError, startDate, endDate;

   auto lisorders = repo->getHL7ObrList(lis::DEV_TEST_HL7, util::getSqlQueryOption(arg, startDate, endDate, queryError));
   if (!queryError.empty())
      alert->error(queryError, web::Alert::LOC_TOAST);

   web::dom::DatatableOpt datatableopt;
   auto page = std::make_shared<web::Page>(httpCtx);
   page->data("pageMenu",         _menuGroup);
   page->data("pageTitle",        "LIS Test Device HL7");
   page->data("pageLang",         "en");
   page->data("pageBodyClass",    "bg-primary");
   page->data("pageId",           "id_lis_testdev_hl7");
   page->data("pageContentTitle", "LIS Test Device HL7");
   page->data("sessExpNotice",    60);
   page->data("sessExpTime",      authResult.expirationTime);
   page->data("pageBreadcrumb",   "LIS / Test Device");
   page->data("datatableId",      "id_table_lisorders");
   page->data("datatable",         datatableopt);
   page->data("dataStartDate",     startDate);
   page->data("dataEndDate",       endDate);
   page->data()["identity"]["userName"]  = authResult.identity.pUser->userName;
   page->data("lisorders", lisorders);

   return page->show("lis/testdev_hl7.tpl", "appview_lis");
}

void ApiLisController::hndlrLisEngineStarted(const std::string& arg)
{
   std::lock_guard<std::mutex> lockGuard(engineStateMutex);
   engineState.threadId = arg;
   _startedTime = std::make_shared<DateTime>();
}

void ApiLisController::hndlrLisEngineStopped(const std::string& arg)
{
   std::lock_guard<std::mutex> lockGuard(engineStateMutex);
   _stoppedTime = std::make_shared<DateTime>();
   engineState.lisTCPConnState = "Disconnected";
}

void ApiLisController::hndlrLisConnectionCreated(const std::string& arg) {}

void ApiLisController::hndlrLisConnected(const std::string& arg)
{
   std::lock_guard<std::mutex> lockGuard(engineStateMutex);
   engineState.lisTCPConnState = "Connected";
}

void ApiLisController::hndlrLisConnectFailed(const std::string& arg)
{
   std::lock_guard<std::mutex> lockGuard(engineStateMutex);
   engineState.lisTCPConnState = "Disconnected";
}

void ApiLisController::hndlrLisSendProgress(double val) {}

void ApiLisController::hndlrLisRecordReady(std::shared_ptr<lis::Record>& record) {}

void ApiLisController::hndlrLisMessageToStorage(std::shared_ptr<lis::Message>& message)
{
   putMessageToDatabase(message);
}

void ApiLisController::hndlrLisMessageReady(std::shared_ptr<lis::Message>& message)
{
   Logger::logI("[lis_ctrl] Handle LIS Message Ready event from LIS Engine");

   std::lock_guard<std::mutex> lockGuard(engineStateMutex);
   engineState.totalMessageReceived++;

   if ( message->vendorProtocolId() == lis::MSG_HL7 ) // or we can use _hl7ProtocolMode
   {
      hndlrLisMessageReady_HL7(message);
   }
   else if (message->vendorProtocolId() == lis::MSG_LIS2A || message->vendorProtocolId().empty())
   {
      // LIS2A Message handled here
      if (_option.activeInstrument == lis::DEV_TEST_LIS1A)
         hndlrLisMessageReady_DevTest_LIS1A(message, engineState);
   }
}

void ApiLisController::hndlrLisCommunicationIdle(const std::string& arg)
{
   if (_hl7ProtocolMode) {
      lisEngine()->sendPendingMessage();
   }
   else
   {
      if (_option.activeInstrument == lis::DEV_TEST_LIS1A)
         hndlrLisCommunicationIdle_DevTest_LIS1A();
   }
}

void ApiLisController::updateEngineStatus()
{
   if (! this->lisEngine())
      return;

   std::lock_guard<std::mutex> lockGuard(engineStateMutex);

   auto lisOpt = this->_option;
   engineState.lisHostId       = lisOpt.hostId;
   engineState.lisHostProvider = lisOpt.hostProvider;
   engineState.lisRole         = lisOpt.role;
   engineState.lisStartupMode  = lisOpt.autoStart ? "Auto" : "Manual" ;
   engineState.lisInstrument   = lisOpt.activeInstrument;

   if (this->lisEngine())
      engineState.lisLinkState = lisEngine()->lisLinkState();
   else
      engineState.lisLinkState = "Unknown";

   if (this->lisEngine() && this->lisEngine()->isStarted())
   {
      bool connected = lisEngine()->isConnected();
      bool tcpServer = lisEngine()->isTCPServerMode();

      bool useSerial = lisOpt.connection.activeConnection == "serial" ? true : false;
      auto portName  = lisOpt.connectionTypes.serialPort.portName;

      engineState.lisRunningState = "Running";
      engineState.lisTcpMode      = tcpServer ? "TCP Server" : (useSerial ? "Serial connection - " + portName : "TCP Client");

      auto startPoint = _startedTime->timePoint();
      auto nowPoint   = DateTime::now().timePoint();
      auto interval   = ( nowPoint - startPoint ).count();
      engineState.lisUptime = util::readMilliseconds(interval);
   }
   else
   {
      engineState.lisRunningState = "Stopped";
      engineState.lisTcpMode      = "N/A";

      if (_stoppedTime)
      {
         auto stopPoint = _stoppedTime->timePoint();
         auto nowPoint  = DateTime::now().timePoint();
         auto interval  = ( nowPoint - stopPoint ).count();
         engineState.lisDownTime = util::readMilliseconds(interval);
      }
      else
         engineState.lisDownTime = "N/A";
   }

   engineState.lisDbInfo       = _dbServiceFactory->lisService()->dbVersionString();
   engineState.appcompilerInfo = tbs::compilerId() + " " + tbs::compilerVersion() + " " + tbs::compilerPlatform();
   engineState.appNameVersion  = "LIS Engine " + tbs::appVersion();
   engineState.appBuildDate    = tbs::appBuildDate();

   // Main application sql connection object
   std::string dbInformation =
      std::visit([&](auto& conn)
         { return conn->versionString(); }, _dbService->sqlConnPtrVariant() );

   std::string dbName =
      std::visit([&](auto& conn)
         { return conn->databaseName(); }, _dbService->sqlConnPtrVariant() );

   engineState.appDbInfo =  dbName + " " + dbInformation;
}

void ApiLisController::putMessageToDatabase(const std::shared_ptr<lis::Message>& message)
{
   if (!_option.result.saveData)
      return;

   Logger::logD("[lis_ctrl] [msg:{}] Storing LIS message into database", message->internalId());

   try
   {
      // Save LIS message into the database asynchronously.
      // We want this function return immediately!!, because we are in Receiving mode ( handling DataLinkStd's onReceiveData event)
      // (see  DataLinkStd::connectionOnReceiveData()), and we must handle EOT from other LIS

      asio::post(*_dbThreadPool,
         [&, this, message]
         {
            auto threadId = [] {
               std::stringstream ss;
               ss << std::this_thread::get_id();
               return ss.str();
            };

            try
            {
                  Logger::logT("[lis_ctrl] [msg:{}] Creating new DbServiceFactory for LIS service on thread id: {}", message->internalId(), threadId());

               // Create new DbServiceFactoryApp instance for LIS
               DbServiceFactory db(_dbOption, _option.activeInstrument);
               auto repo = db.lisService();

               lis::svc::DbStoreResultPtr dbStoreResult = std::make_shared<lis::svc::DbStoreResult>();
               bool success = repo->saveLisData(message, dbStoreResult);
               if (success) {
                    Logger::logD("[lis_ctrl] [msg:{}] LIS message successfully stored in database", message->internalId());
               }

               Logger::logT("[lis_ctrl] [msg:{}] DB thread id:{} done", message->internalId(), threadId() );
            }
            catch(const std::exception& ex)
            {
               Logger::logE("[lis_ctrl] [msg:{}] DB thread id:{} error: {}", message->internalId(), threadId(), ex.what() );

               DateTime curr;
               EngineError error;
               error.task      = "Save LIS message to database";
               error.message   = ex.what();
               error.timestamp = curr.toUnixTimeMiliSeconds();
               error.source    = "LIS DB";

               engineState.lisErrors.emplace_back(std::move(error));
            }
      });
   }
   catch(const std::exception& ex) {
      Logger::logE("[lis_ctrl] [msg:{}] Failed to schedule LIS message storage to database thread pool: {}", message->internalId(), ex.what());
   }
}



} // namespace lis
} // namespace tbs