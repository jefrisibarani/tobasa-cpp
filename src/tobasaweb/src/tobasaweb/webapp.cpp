#include <tobasa/config.h>
#include <new>
#include "tobasaweb/multi_logger.h"
#include "tobasaweb/credential_info.h"
#include "tobasaweb/webapp.h"
#include "tobasaweb/db_migrations/001_initial_base_schema.h"

namespace tbs {
namespace web {

Webapp::Webapp()
   : _ioContext(1)
{
   _pWebService = new web::WebService();
   _pWebService->setIoContext(_ioContext);

   _myAgent = std::make_shared<WebappAgent>();
   _myAgent->_pApp = this;
   _myAgent->_status.appThreadId = util::threadId(std::this_thread::get_id());

   // Add Base database migration
   _dbMigration.add<dbm::InitialBaseSchema>();
}

void Webapp::cleanup()
{
   if (_stopped)
      return;

   if (_dbService)
      _dbService.reset();

   // reset Loggers' log target
   Logger::destroyInternalLogSink() ;

   _stopped = true;
   std::cout << "[webapp] Webapp resources cleaned\n";
}

Webapp::~Webapp()
{
   cleanup();

   std::cout << "[webapp] Webapp destroyed\n";
}

void Webapp::useWebService(web::WebServiceBase* service)
{
   if (_pWebService) {
      delete _pWebService;
   }

   _pWebService = service;
   _pWebService->setIoContext(_ioContext);
}

void Webapp::useDbService(sql::DbServiceFactoryPtr service)
{
   _dbService = service;
   _useCustomDbService = true;
}

/** Use Authentication middleware.
 * @param builder Options builder functor for middleware
 * Setup a middleware to activate AuthResult as Http Request user data.
 * Setup AuthenticationMiddleware
 */
void Webapp::useAuthentication(web::AuthenticationMiddlewareOptionBuilder builder)
{
   // Add AuthResult middleware
   // this middleware setup AuthResult as HttpContext user data, required by
   // Authentication and Authorization middleware
   this->addMiddleware(
      [](const http::HttpContext& context, const http::RequestHandler& next)
      {
         if ( !context->userData().has_value() ) {
            context->userData( std::make_any<web::AuthResult>() );
         }

         //context->response()->addHeader("X-Processed-By",  "Authresult Middleware");
         return next(context);
      }
      , "AuthResult"
   );


   // Add Authentication middleware
   auto factory = web::makeMiddleware<web::AuthenticationMiddleware>();
   if (builder)
   {
      web::AuthenticationMiddlewareOption option;
      builder(option); // build the options
      factory->resultBuilder(std::move(option.resultBuilder));
      auto middlewareInstance = std::static_pointer_cast<web::AuthenticationMiddleware>(factory->middleWare());
      if (middlewareInstance)
         middlewareInstance->option(std::move(option));
   }

   this->addMiddleware( factory );
}

/** Use Authorization middleware.
 * @param builder Options builder functor for middleware
 */
void Webapp::useAuthorization(web::AuthorizationMiddlewareOptionBuilder builder)
{
   auto factory = web::makeMiddleware<web::AuthorizationMiddleware>();
   if (builder)
   {
      web::AuthorizationMiddlewareOption option;
      builder(option); // build the options
      factory->resultBuilder(std::move(option.resultBuilder));
      auto middlewareInstance = std::static_pointer_cast<web::AuthorizationMiddleware>(factory->middleWare());
      if (middlewareInstance)
         middlewareInstance->option(std::move(option));
   }

   this->addMiddleware( factory );
}

/** Use Session middleware.
 * @param builder Options builder functor for middleware
 */
void Webapp::useSession(web::SessionMiddlewareOptionBuilder builder)
{
   auto factory = web::makeMiddleware<web::SessionMiddleware>();
   if (builder)
   {
      web::SessionMiddlewareOption option;
      builder(option); // build the options
      factory->resultBuilder(std::move(option.resultBuilder));
      auto middlewareInstance = std::static_pointer_cast<web::SessionMiddleware>(factory->middleWare());
      if (middlewareInstance)
         middlewareInstance->option(std::move(option));
   }

   this->addMiddleware( factory );
}

/** Use Session middleware.
 * @param builder Options builder functor for middleware
 */
void Webapp::useMultipart(web::MultipartMiddlewareOptionBuilder builder)
{
   // Check if Server multipart parser is enabled.
   if ( _appOption.httpServer.enableMultipartParsing)
      return; // Parse multipart inside Server

   auto factory = web::makeMiddleware<web::MultipartMiddleware>();
   
   // Create default value for option
   web::MultipartMiddlewareOption option;
   option.temporaryDir = _appOption.httpServer.temporaryDir;

   if (builder)
      builder(option); // use option from builder

   auto middlewareInstance = std::static_pointer_cast<web::MultipartMiddleware>(factory->middleWare());
   if (middlewareInstance)
      middlewareInstance->option(std::move(option));

   this->addMiddleware( factory );
}

void Webapp::useRouter(web::RouterFactoryPtr factory)
{
   _pWebService->useRouter(factory);
}

void Webapp::addController(std::shared_ptr<web::ControllerFactoryBase> factory)
{
   _pWebService->addController(factory);
}

void Webapp::addMiddleware(web::MiddlewareFactoryPtr factory)
{
   _pWebService->addMiddleware(factory);
}

void Webapp::addMiddleware(http::RequestHandlerChained handler, const std::string& name)
{
   auto factory = std::make_shared<tbs::web::MiddlewareFactory<tbs::web::AutoMiddleware>>(handler);
   factory->middelwareName(name);
   _pWebService->addMiddleware(factory);
}

bool Webapp::loadConfig(const std::string& configFile, nonstd::span<const unsigned char> charData)
{
   std::cout << "[webapp] Setting up aplication configuration..." << "\n";
   
   _configFile = configFile;

   try
   {
      if ( _configFile.length() == 0 )
         _configFile = "appsettings.json"; // load from default file inside application binary dir

      // load configurations
      Config::get().load(_configFile, charData);

      if ( Config::get().valid() )
      {
         auto globalSalt = Config::getOption<std::string>("securitySalt");
         auto salt = tbs::Config::tryGetNestedOption<std::string>("webapp.dbConnection.securitySalt", "N/A");
         // We have to set security salt for webapp.dbConnection
         Config::setNestedOption("webapp.dbConnection.securitySalt", globalSalt);
         
         // Now init our App Option
         _appOption = Config::getOption<conf::Webapp>("webapp");

         // setup Tobasa Logger actual target
         auto logOption = Config::getOption<log::conf::Logging>("logging");
         Logger::setTarget(new log::MultiLogger(std::move(logOption)));

         // Check temporary dir
         // -------------------------------------------------------
         if (_appOption.httpServer.temporaryDir.empty()) {
            _appOption.httpServer.temporaryDir = path::temporaryDir();
         }

         auto tmpDir = path::resolveExecutableRelativePath(_appOption.httpServer.temporaryDir);
         if (tmpDir.empty()) {
            throw AppException("Invalid Http Server temporary folder");
         }
         _appOption.httpServer.temporaryDir = tmpDir;

         if ( !path::exists(_appOption.httpServer.temporaryDir) )
         {
            if ( !path::createDir(_appOption.httpServer.temporaryDir) )
               throw AppException("Could not create Http Server temporary folder " + _appOption.httpServer.temporaryDir);
         }


         // Check tls asset files
         // -------------------------------------------------------
         auto tlsCertFile = _appOption.httpServer.tls.certificateChainFile;
         _appOption.httpServer.tls.certificateChainFile = path::resolveExecutableRelativePath(tlsCertFile);

         auto tlsPrivate = _appOption.httpServer.tls.privateKeyFile;
         _appOption.httpServer.tls.privateKeyFile = path::resolveExecutableRelativePath(tlsPrivate);
         
         auto tlsTmpDh = _appOption.httpServer.tls.tmpDhFile;
         _appOption.httpServer.tls.tmpDhFile = path::resolveExecutableRelativePath(tlsTmpDh);

         for (auto& hostCert : _appOption.httpServer.tls.hostCertificates)
         {
            auto certFile = hostCert.certificateChainFile;
            hostCert.certificateChainFile = path::resolveExecutableRelativePath(certFile);

            auto privKey = hostCert.privateKeyFile;
            hostCert.privateKeyFile = path::resolveExecutableRelativePath(privKey);
         }

         _configOk = true;
         std::cout << "[webapp] Configuration loaded..." << "\n";
         return true;
      }
   }
   catch (const Json::exception& ex) {
      std::cerr << "[webapp] Configuration exception: " << ex.what() << "\n";
   }
   catch(const AppException& ex) {
      std::cerr << "[webapp] Configuration exception: " << ex.what() << "\n";
   }
   catch(const std::exception& ex) {
      std::cerr << ex.what() << '\n';
   }

   return false;
}

bool Webapp::configValid()
{
   return _configOk;
}

int Webapp::start(size_t ioPoolSize, size_t workerPoolSize)
{
   try
   {
      Logger::logI("[webapp] Starting Tobasa webapp...");

      if (! _configOk)
         return 1;

      Session::clearOldSessionFiles();

      // Apply correct limits
      setThreadPoolSize(ioPoolSize, workerPoolSize); // and db connection pool size

      if (_dbService == nullptr)
      {
         if (_useCustomDbService)
         {
            Logger::logE("[webapp] Invalid database instance");
            return 1;
         }
         else
         {
            _dbService = std::make_shared<sql::DbServiceFactory>();
            _dbService->addConnectorOption("MainAppDbOption", _appOption.dbConnection);
         }
      }

      if (_dbService == nullptr)
         return 1;

      _dbService->setConnectionPoolSize(_dbConnPoolSize);
      _pWebService->useDbService(_dbService);

      try
      {
         dbm::runDbMigration(&_appOption.dbConnection, _dbMigration);

         // connect to database
         if (! _dbService->connected())
         {
            Logger::logE("[webapp] No connection to database");
            return 1;
         }
      }
      catch(std::exception& e) {
         Logger::logE("[webapp] {}", e.what());
      }

      _pWebService->setupHandlers();

      runHttpServer();
      return 0;
   }
   catch (Json::exception& e)
   {
      disconnectDb();

      Logger::logE("[webapp] Configuration exception: {}", e.what());
      return 1;
   }
   catch (std::exception& e)
   {
      disconnectDb();

      Logger::logE("[webapp] Application exception : {}", e.what());
      return 1;
   }
   catch (...)
   {
      Logger::logE("[webapp] start() Unknown exception");
   }

   return 0;
}


void Webapp::joinIoThreads()
{
   // Avoid joining the current thread (this can happen if shutdown
   // is called from one of the io_context threads when handling SIGINT).
   auto selfId = std::this_thread::get_id();
   for (auto& thread : _threadPool)
   {
      if (!thread.joinable())
         continue;

      if (thread.get_id() == selfId)
         continue; // cannot join ourselves

      thread.join();
   }
}

void Webapp::shutdown() noexcept
{
   bool expected = false;
   if (!_shutdownCalled.compare_exchange_strong(expected, true))
      return;

   try
   {
      _ioContext.stop();

      if (_workerPoolSize > 0 && _workerPool)
         _workerPool->join();

      if (_ioPoolSize > 0)
         joinIoThreads();

      callOnStopFunctor();

      cleanup();
   }
   catch (const std::exception& ex)
   {
      Logger::logE("[webapp] shutdown() exception: {}", ex.what());
   }
   catch (...)
   {
      Logger::logE("[webapp] shutdown() unknown exception");
   }
}

void Webapp::runIoContextOnThreadPool(size_t poolSize)
{
   try
   {
      auto mainThreadId = util::threadId(std::this_thread::get_id());
      auto work {asio::make_work_guard(_ioContext)};

      for (std::size_t i = 0; i < poolSize; ++i)
      {
         _threadPool.emplace_back(
            std::thread{ [this,mainThreadId] {

               auto ioThreadId = util::threadId(std::this_thread::get_id());
               Logger::logD("[webapp] webapp thread id {} created, callled from thread id {}", ioThreadId, mainThreadId );

               _ioContext.run();

               Logger::logD("[webapp] webapp thread id {} ended", ioThreadId );
            }}
         );
      }
   }
   catch (const std::exception&)
   {
      _ioContext.stop();
      joinIoThreads();
      throw;
   }

   joinIoThreads();
}

web::RouterPtr Webapp::router()
{
   return _pWebService->router();
}

asio::io_context& Webapp::ioContext() { return _ioContext; }

bool Webapp::disconnectDb()
{
   return true;
}

void Webapp::callOnStartFunctor() noexcept
{
   if (_onStartHandler)
   {
      OnStartHandler fn{ std::move(_onStartHandler) };
      fn();
   }
}

void Webapp::callOnStopFunctor() noexcept
{
   if (_onStopHandler)
   {
      OnStopHandler fn{ std::move(_onStopHandler) };
      fn();
   }
}

void Webapp::setThreadPoolSize(size_t ioPoolSize, size_t workerPoolSize)
{
   auto& httpdconf = _appOption.httpServer;

   const size_t hw = std::thread::hardware_concurrency();
   const size_t fallback = hw ? hw : 4;
   const size_t maxThreads = hw ? hw * 4 : 128;

   // IO POOL 
   size_t io = 0;

   if (ioPoolSize > 0)
      io = ioPoolSize;
   else if (ioPoolSize == 0)
      io = httpdconf.ioPoolSize;

   if (io == 0) {
      _ioPoolSize = 0;  // 0 means disabled
   }
   else
   {
      io = std::min(io, maxThreads);
      _ioPoolSize = io;
   }

   // WORKER POOL 
   size_t worker = 0;

   if (workerPoolSize > 0)
      worker = workerPoolSize;
   else if (workerPoolSize == 0)
      worker = httpdconf.workerPoolSize; 

   if (worker == 0) {
      _workerPoolSize = 0;  // 0 means disabled
   }
   else
   {
      worker = std::min(worker, maxThreads);
      _workerPoolSize = worker;
   }

   _dbConnPoolSize = _appOption.dbConnectionPoolSize;
   if (_dbConnPoolSize < 4)
      _dbConnPoolSize = _workerPoolSize;

   Logger::logI("[webapp] IO threads: {}, Worker threads: {}, DB Connection pools: {}", _ioPoolSize, _workerPoolSize, _dbConnPoolSize );
}

void Webapp::runHttpServer()
{
   auto& httpdconf = _appOption.httpServer;

   _workerPool = std::make_unique<asio::thread_pool>(_workerPoolSize);

   using namespace std::placeholders;

   // ---------------------------------
   // HTTP Server
   // ---------------------------------
   http::Settings httpSetting;
   httpSetting
      .logVerbose( httpdconf.logVerbose)
      .port( httpdconf.port)
      .address( httpdconf.address )
      .maxRequestsPerConnection(httpdconf.maxRequestsPerConnection)
      .timeoutRead( httpdconf.timeoutRead)
      .timeoutWrite( httpdconf.timeoutWrite)
      .timeoutProcessing( httpdconf.timeoutProcessing)
      .readBufferSize( httpdconf.readBufferSize)
      .sendBufferSize( httpdconf.sendBufferSize )
      .maxHeaderSize( httpdconf.maxHeaderSize)
      .enableMultipartParsing(httpdconf.enableMultipartParsing)
      .temporaryDir(httpdconf.temporaryDir)
      .useCompression(httpdconf.compression.enable)
      .compressionEncoding(httpdconf.compression.encoding)
      .compressionMimeTypes(httpdconf.compression.mimetypes)
      .compressionMinimalLength(httpdconf.compression.minimalLength)
      .useRateLimiter(httpdconf.useRateLimiter)
      .rateLimiterMaxRequests(httpdconf.rateLimiterMaxRequests)
      .rateLimiterWindowDuration(httpdconf.rateLimiterWindowDuration)
      .rateLimiterBlockDuration(httpdconf.rateLimiterBlockDuration)
      .rateLimiterMaxViolations(httpdconf.rateLimiterMaxViolations)
      ;

   http::PlainServer plainServer(_ioContext, std::move(httpSetting), _logger);
   plainServer.statusPageBuilder( _pWebService->serverStatusPageBuilder() );
   
   // set server request handler
   if (_workerPoolSize > 0)
   {
      plainServer.requestHandler( 
         [this](const http::HttpContext& context) {
            return handleRequest(context);
         });
   }
   else {
      plainServer.requestHandler( _pWebService->serverHttpRequestHandler() );
   }   

   // ---------------------------------
   // HTTPS Server
   // ---------------------------------
   http::SettingsTls tlsSetting;
   tlsSetting
      .serverMode(true)
      .hostCertificates(http::conf::toHostCertificates(httpdconf.tls.hostCertificates))
      .certificateChainFile( httpdconf.tls.certificateChainFile)
      .privateKeyFile( httpdconf.tls.privateKeyFile)
      .tmpDhFile( httpdconf.tls.tmpDhFile)
#ifdef TOBASA_HTTP_USE_HTTP2
      .http2Enabled(httpdconf.http2Enabled)
      .logVerboseHttp2(httpdconf.logVerboseHttp2)
#endif
      .logVerbose( httpdconf.logVerbose)
      .port( httpdconf.portHttps)
      .address( httpdconf.address )
      .maxRequestsPerConnection(httpdconf.maxRequestsPerConnection)
      .timeoutRead( httpdconf.timeoutRead)
      .timeoutWrite( httpdconf.timeoutWrite)
      .timeoutProcessing( httpdconf.timeoutProcessing)
      .readBufferSize( httpdconf.readBufferSize)
      .sendBufferSize( httpdconf.sendBufferSize )
      .maxHeaderSize( httpdconf.maxHeaderSize)
      .enableMultipartParsing(httpdconf.enableMultipartParsing)
      .temporaryDir(httpdconf.temporaryDir)
      .useCompression(httpdconf.compression.enable)
      .compressionEncoding(httpdconf.compression.encoding)
      .compressionMimeTypes(httpdconf.compression.mimetypes)
      .compressionMinimalLength(httpdconf.compression.minimalLength)
      .useRateLimiter(httpdconf.useRateLimiter)
      .rateLimiterMaxRequests(httpdconf.rateLimiterMaxRequests)
      .rateLimiterWindowDuration(httpdconf.rateLimiterWindowDuration)
      .rateLimiterBlockDuration(httpdconf.rateLimiterBlockDuration)
      .rateLimiterMaxViolations(httpdconf.rateLimiterMaxViolations)
      .defaultTlsAssetCallback( [this] (http::TlsAsset asset ) {
         if (_defaultTlsAssetCallback)
            return _defaultTlsAssetCallback(asset);
         else
            return nonstd::span<const unsigned char>();
      })
      ;

   http::SecureServer secureServer(_ioContext, std::move(tlsSetting), _logger);
   secureServer.statusPageBuilder( _pWebService->serverStatusPageBuilder() );
   
   // set server request handler
   if (_workerPoolSize > 0)
   {
      secureServer.requestHandler( 
         [this](const http::HttpContext& context) {
            return handleRequest(context);
         });
   }
   else {
      secureServer.requestHandler( _pWebService->serverHttpRequestHandler() );
   }


   _myAgent->_pSrvPlain        = &plainServer;
   _myAgent->_pSrvSecure       = &secureServer;
   _myAgent->_status.httpPort  = httpdconf.port;
   _myAgent->_status.httpsPort = httpdconf.portHttps;


   // Starts server in async
   // -------------------------------------------------------
   std::exception_ptr exceptionCaught;
   asio::signal_set breakSignals{ _ioContext, SIGINT };
   breakSignals.async_wait(
      [&,this](const asio::error_code & ec, int)
      {
         Logger::logI("[webapp] Receive stop signal");
         if (!ec)
         {
            asio::post(
               plainServer.executor(),
               [this, &plainServer, &secureServer, &exceptionCaught] {
                  try
                  {
                     if (_appOption.httpServer.runHttpsOnly)
                        secureServer.stop();
                     else
                     {
                        plainServer.stop();
                        secureServer.stop();
                     }

                     shutdown();
                  }
                  catch (...)
                  {
                     exceptionCaught = std::current_exception();
                     shutdown();
                  }
               });
         }
      });

   asio::post(
      plainServer.executor(),
      [this, &plainServer, &secureServer, &exceptionCaught] {
         try
         {
            if (_appOption.httpServer.runHttpsOnly)
            {
               _myAgent->_status.httpsOnly = true;
               secureServer.start();
            }
            else
            {
               _myAgent->_status.httpsOnly = false;
               plainServer.start();
               secureServer.start();
            }
            
            _myAgent->_status.startedTime = std::make_shared<DateTime>();
         }
         catch (...)
         {
            exceptionCaught = std::current_exception();
            shutdown();
         }
      });

   callOnStartFunctor();

   if (_ioPoolSize <= 0)
      _ioContext.run();
   else
      runIoContextOnThreadPool(_ioPoolSize);

   if (_workerPoolSize > 0)
      _workerPool->join();

   if (exceptionCaught)
      std::rethrow_exception( exceptionCaught );
}

void Webapp::serverStatusPageBuilder(http::StatusPageBuilder renderer)
{
   _pWebService->serverStatusPageBuilder(renderer);
}

http::StatusPageBuilder Webapp::serverStatusPageBuilder()
{
   return _pWebService->serverStatusPageBuilder();
}

bool Webapp::dbConnected()
{
   if (_dbService && _dbService->connected())
      return true;
   else
      return false;
}

bool Webapp::reconnectDb(sql::DbServiceFactoryPtr service)
{
   try
   {
      if ( service && service->connected() )
      {
         _dbService = service;
         _pWebService->reconfigureDb(_dbService);

         return true;
      }
      else
      {
         _dbService->disconnect();
         _dbService->connect();
         _pWebService->reconfigureDb(_dbService);
         return true;
      }
   }
   catch(std::exception& e) {
      Logger::logE("[webapp] Database service error: {}", e.what());
   }

   return false;
}

std::shared_ptr<WebappAgent> Webapp::agent()
{
   return _myAgent;
}

void Webapp::defaultTlsAssetCallback(http::DefaultTlsAssetCallback cb)
{
   _defaultTlsAssetCallback = std::move(cb);
}

http::RequestStatus Webapp::handleRequest(const http::HttpContext& context)
{
   asio::post(*_workerPool,
      [this, ctx=context]()
      {
         try
         {
            auto handler = _pWebService->serverHttpRequestHandler();
            auto resStatus = (*handler)(ctx);

            // Return to IO thread to send response
            asio::post(_ioContext,
               [ctx=ctx, resStatus]() {
                  ctx->complete(resStatus);
               });
         }
         catch (...)
         {
            Logger::logE("[webapp] http request handler error");
            
            asio::post(_ioContext,
               [ctx=ctx](){
                  ctx->response()->httpStatus( tbs::http::StatusCode::INTERNAL_SERVER_ERROR );
                  ctx->response()->setHeaderContentType("text/html");
                  ctx->complete(tbs::http::RequestStatus::handled);
               });
         }
      });

   return tbs::http::RequestStatus::async; // important
}

} // namespace web
} // namespace tbs