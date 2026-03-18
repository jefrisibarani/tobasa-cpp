#pragma once

#include <tobasa/non_copyable.h>
#include <tobasa/logger_tobasa.h>
#include <tobasa/span.h>
#include <memory>
#include <tobasahttp/server/http_server.h>
#include <tobasahttp/server/settings_tls.h>
#include <tobasasql/database_service_factory.h>
#include "tobasaweb/authentication_middleware.h"
#include "tobasaweb/authorization_middleware.h"
#include "tobasaweb/session_middleware.h"
#include "tobasaweb/multipart_middleware.h"
#include "tobasaweb/router.h"
#include "tobasaweb/web_service.h"
#include "tobasaweb/settings_webapp.h"
#include "tobasaweb/settings_log.h"
#include "tobasaweb/webapp_agent.h"
#include "tobasaweb/db_migration.h"

namespace tbs {
namespace web {

/**
 * \defgroup WEB Web service application library
 */


/**
 * \ingroup WEB
 * \brief Web application builder.
 * \details Prepares and builds a web service application.
 */
class Webapp : private NonCopyable
{
   friend class WebappAgent;

private:
   asio::io_context         _ioContext;
   web::WebServiceBase*     _pWebService {nullptr};
   sql::DbServiceFactoryPtr _dbService   {nullptr};
   bool                     _configOk    {false};
   conf::Webapp             _appOption;
   log::TobasaLogger        _logger;

   /// Pool of threads to run all of the io_contexts. 
   std::vector<std::thread> _threadPool;
   size_t                   _ioPoolSize {0};

   /// Worker thread pool for HTTP request handler
   std::unique_ptr<asio::thread_pool> _workerPool;
   size_t                   _workerPoolSize {0};

   int                      _dbConnPoolSize {0};
   
   std::string              _configFile;
   bool _useCustomDbService {false};
   bool _stopped            {false};
   std::atomic<bool>        _shutdownCalled{false};

   dbm::MigrationJob        _dbMigration;

protected:
   using OnStopHandler      = std::function<void(void)>;
   using OnStartHandler     = std::function<void(void)>;

   OnStopHandler            _onStopHandler;
   OnStartHandler           _onStartHandler;

   http::DefaultTlsAssetCallback  _defaultTlsAssetCallback;

   std::shared_ptr<WebappAgent>   _myAgent = nullptr;

public:

   /**
    * \brief Constructs a Webapp instance with an IO context and a web service.
    * Initializes a Webapp object, setting up an IO context and creating a web service instance.
    * Establishes the web service within the application, configuring it with the IO context.
    */
   Webapp();

   /**
    * Destroys the Webapp instance.
    */   
   ~Webapp();

   /**
    * \brief Sets a function to execute when the Webapp starts.
    * \param func A function to be executed when the Webapp starts.
    */
   template<typename Func> 
   void onStart(Func && func)
   {
      _onStartHandler = std::move(func);
   }

   /**
    * \brief Sets a function to execute when the Webapp stops.
    * \param func A function to be executed when the Webapp stops.
    */
   template<typename Func> 
   void onStop(Func && func)
   {
      _onStopHandler = std::move(func);
   }

   /**
    * \brief Sets a custom web service for use by the WebApp.
    * To utilize a custom web service, call this function immediately after instantiating the WebApp.
    * \param service Pointer to a custom web service implementing web::WebServiceBase.
    */
   virtual void useWebService(web::WebServiceBase* service);

   /**
    * \brief Sets a custom web service for use by the Webapp.
    * \param service Pointer to a custom web service implementing web::WebServiceBase.
    */
   virtual void useDbService(sql::DbServiceFactoryPtr service);

   /**
    * \brief Sets up Authentication middleware for the Webapp.
    * \details Activates AuthResult as HTTP request user data and configures AuthenticationMiddleware.
    * This method enables authentication functionality within the application.
    * Use the provided builder to customize middleware options.
    * \param builder Options builder functor for middleware configuration.
    */
   virtual void useAuthentication(web::AuthenticationMiddlewareOptionBuilder builder = nullptr);

   /**
    * \brief Sets up Authorization middleware for the Webapp.
    * Configures AuthorizationMiddleware, enabling authorization functionality within the application.
    * Use the provided builder to customize middleware options.
    * This middleware controls access rights based on defined permissions.
    * \param builder Options builder functor for middleware configuration.
    */
   virtual void useAuthorization(web::AuthorizationMiddlewareOptionBuilder builder = nullptr);

   /**
    * \brief Sets up Session middleware for the Webapp.
    * \details Configures SessionMiddleware, enabling session management within the application.
    * Use the provided builder to customize middleware options for session handling.
    * This middleware maintains session states and handles session-related operations.
    * \param builder Options builder functor for middleware configuration.
    */
   virtual void useSession(web::SessionMiddlewareOptionBuilder builder = nullptr);

   /**
    * \brief Sets up Multipart middleware for the Webapp.
    * \details Configures MultipartMiddleware, enabling multipart parsing inside a middleware
    * Note that Server's setting "enableMultipartParsing" takes priority.
    * If Server set with "enableMultipartParsing" is true, Multipart parsing will be done inside HttpServer
    * thus ignoring this middleware.
    * 
    * \param builder Options builder functor for middleware configuration.
    */
   virtual void useMultipart(web::MultipartMiddlewareOptionBuilder builder = nullptr);

   /**
    * \brief Sets the router for the Webapp.
    * \param factory Shared pointer to a router factory.
    */   
   virtual void useRouter(web::RouterFactoryPtr factory);

   /**
    * \brief Adds a controller factory to the Webapp.
    * \param factory Shared pointer to a controller factory
    */
   virtual void addController(std::shared_ptr<web::ControllerFactoryBase> factory);

   /**
    * \brief Adds a middleware factory to the Webapp.
    * \param factory Shared pointer to a middleware factory
    */
   virtual void addMiddleware(web::MiddlewareFactoryPtr factory);

   /**
    * \brief Adds a chained request handler as middleware to the Webapp.
    * \param handler Chained request handler defining middleware functionalities.
    */
   virtual void addMiddleware(http::RequestHandlerChained handler, const std::string& name={});

   /**
    * \brief Loads and applies the application's configuration settings.
    * \details Loads and sets up the necessary configurations for the web service application.
    * If no configuration file is specified, it defaults to "appsettings.json".
    * Upon successful loading and validation of configurations, the method sets up
    * the application options, logger, and database connection settings.
    * Returns true on successful loading; otherwise, returns false.
    * \return A boolean indicating the success of the configuration loading process.
    */
   virtual bool loadConfig(const std::string& configFile, nonstd::span<const unsigned char> charData={});

   /**
    * \brief Checks if the application's configuration is valid.
    * \return A boolean indicating the validity of the application's configuration.
    */
   virtual bool configValid();

   /**
    * \brief Initiates the startup sequence for the Tobasa web application.
    * \details Starts the Tobasa web application, initializing necessary components and services.
    * Checks the validity of the configuration and database service, then initializes
    * the SQL driver and connects to the database. Sets up web service handlers and runs
    * the HTTP server based on the provided thread pool size.
    * Returns 0 on successful startup, 1 on failure during initialization or execution.
    * \param ioPoolSize Size of the thread pool for running io_context.
    * \param workerPoolSize Size of the thread pool for running http request handler.
    * \return An integer representing the exit status.
    */
   virtual int start(size_t ioPoolSize=0, size_t workerPoolSize=0);

   /**
    * \brief Runs io_context on a thread pool.
    * \details Sets up and manages a thread pool to handle concurrent tasks within the Tobasa web application.
    * Each thread in the pool manages the IO context to handle asynchronous operations.
    * Logs thread creation, runs the IO context for each thread, and waits for their completion.
    * Handles exceptions and stops the IO context in case of errors during thread pool setup.
    * \param poolSize Size of the thread pool for executing the application.
    */
   void runIoContextOnThreadPool(size_t poolSize);

   /**
    * \brief Retrieves the router used by the Webapp.
    * \return Shared pointer to the router instance.
    */
   web::RouterPtr router();

   /**
    * \brief Retrieves the pointer to the web service object.
    * \return Pointer to the web service object used by the Webapp.
    */   
   web::WebServiceBase* webService() {return _pWebService;}

   /**
    * \brief Checks the status of the database connection.
    * \details Returns true if the web service application is currently connected to the database.
    * Otherwise, returns false to indicate that there is no active database connection.
    * \return A boolean indicating the database connection status.
    */
   virtual bool dbConnected();

   /**
    * \brief Sets up a custom server status page builder for the Webapp.
    * \param renderer The status page builder defining server status page rendering.
    */
   virtual void serverStatusPageBuilder(http::StatusPageBuilder renderer);

   /**
    * \brief Retrieves the server status page builder used by the Webapp.
    * \return The status page builder for rendering the server status page.
    */
   virtual http::StatusPageBuilder serverStatusPageBuilder();

   /**
    * \brief Attempts to reconnect the web service to the database.
    * \details Tries to re-establish the connection to the database using the provided database service.
    * If successful, updates the internal database service and reconfigures the web service.
    * \param service Shared pointer to a database service for reconnection.
    * \return A boolean indicating the success of the database reconnection attempt.
    */
   virtual bool reconnectDb(sql::DbServiceFactoryPtr service=nullptr);

   std::shared_ptr<WebappAgent> agent();

   void defaultTlsAssetCallback(http::DefaultTlsAssetCallback cb);
   
   dbm::MigrationJob& migrationJob() { return _dbMigration; }


protected:
   
   /**
    * \brief Returns the IO context used by the Webapp.
    * \return Reference to the IO context.
    */   
   asio::io_context& ioContext();

   /**
    * \brief Calls the stored functor associated with the "on start" event.
    * \details Invokes the functor stored for the "on start" event if it's available.
    * Executes the associated function to handle operations when the application starts.
    * \see onStart()
    */   
   void callOnStartFunctor() noexcept;

   /**
    * \brief Calls the stored functor associated with the "on stop" event.
    * \details Invokes the functor stored for the "on stop" event if it's available.
    * Executes the associated function to handle operations when the application stops.
    * \see onStop()
    */
   void callOnStopFunctor() noexcept;

   /**
    * \brief Sets up and manages HTTP and HTTPS servers for the Webapp.
    * \details Configures and initiates both HTTP and HTTPS servers within the application.
    * Manages server settings, including ports, addresses, timeouts, and session buffer size.
    * Handles start and stop functionalities for both servers asynchronously,
    * listening for stop signals to gracefully shut down the servers.
    * Executes onStart and onStop functions accordingly during server operation.
    * Utilizes a thread pool if specified; otherwise, runs the IO context directly.
    */
   void runHttpServer();

   http::RequestStatus handleRequest(const http::HttpContext& context);

   bool disconnectDb();

   void setThreadPoolSize(size_t ioPoolSize, size_t workerPoolSize);

   void joinIoThreads();

   void shutdown() noexcept;

   /// Cleans up resources used by the Webapp.
   void cleanup();
};

} // namespace web
} // namespace tbs