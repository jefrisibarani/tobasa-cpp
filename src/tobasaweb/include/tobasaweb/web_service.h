#pragma once

#include "tobasaweb/router.h"
#include "tobasaweb/middleware_manager.h"
#include "tobasaweb/web_service_base.h"
#include "tobasaweb/controller_factory.h"

namespace tbs {
namespace web {

/** \addtogroup WEB
 * @{
 */


/** 
 * WebService
 * Encapsulates functionalities for managing web services, 
 * controllers, middlewares, and related configurations 
 */
class WebService
   : public WebServiceBase
{
private:
   std::vector<ControllerFactoryPtr> _controllerFactories;
   std::vector<MiddlewareFactoryPtr> _middlewareFactories;
   RouterFactoryPtr                  _routerFactory = nullptr;
   sql::DbServiceFactoryPtr          _dbService     = nullptr;

   /// asio's io_context
   asio::io_context*                 _pIoContext    = nullptr;

   /// Request Handler manager for server, and HTTP server request handler
   MiddlewareManager                 _srvhandler;

   http::StatusPageBuilder           _statusPageBuilder ;

public:
   /**
    * @brief Constructs a WebService object.
    * @details Initializes the WebService by creating a default router and setting up the server handler.
    */
   WebService();

   /**
    * @brief Destructs the WebService object.
    */
   virtual ~WebService();

   /**
    * @brief Get the router instance.
    * This method returns the router instance configured within the WebService through RouterFactory.
    */
   virtual RouterPtr router();

   /// Get a reference to the Asynchronous I/O (asio) context.
   virtual asio::io_context& ioContext();

   /// Sets the DatabaseService for the WebService.
   virtual void useDbService(sql::DbServiceFactoryPtr service);

   /// Sets the RouterFactory for the WebService and configures the server handler with the router.
   virtual void useRouter(RouterFactoryPtr factory);

   /**
    * @brief Adds a ControllerFactory to the WebService.
    * Appends the provided ControllerFactory shared pointer to the list of 
    * ControllerFactories within the WebService.
    * @param factory
    */
   virtual void addController(ControllerFactoryPtr factory);

   /**
    * @brief Adds a MiddlewareFactory to the WebService.
    * Appends the provided MiddlewareFactory pointer to the list of 
    * MiddlewareFactories within the WebService.
    * @param factory
    */
   virtual void addMiddleware(MiddlewareFactoryPtr factory);

   virtual void addMiddleware(http::RequestHandlerChained handler, const std::string& name={});

   /// Sets up all HTTP request handlers, middleware, and router request handler.
   virtual void setupHandlers();

   /**
    * @brief Retrieves the HTTP server request handler.
    * Creates and configures an HTTP server request handler by 
    * binding it to the MiddlewareManager's invoke method.
    * @note This method is designed to be called only once.
    */  
   virtual std::unique_ptr<tbs::http::RequestHandler> serverHttpRequestHandler();

   /**
    * @brief Sets the status page builder for the HTTP server in the WebService.
    * Assigns the provided status page renderer to the WebService's internal status page builder.
    * @param renderer The status page builder to be set for the HTTP server.
    */
   virtual void serverStatusPageBuilder(http::StatusPageBuilder renderer);

   /**
    * @brief Retrieves the status page builder configured for the HTTP server in the WebService.
    */
   virtual http::StatusPageBuilder serverStatusPageBuilder();

   /**
    * @brief Stores the asio::io_context instance created in the WebApp.
    * @param ioContext A reference to the asio::io_context instance to be stored.
    */
   virtual void setIoContext(asio::io_context& ioContext);

   /**
    * @brief Resets the DatabaseService and AuthDbRepo.
    * In case of a lost connection to the database, this method allows resetting
    * and reconfiguring all database-related services within the WebService.
    * @param dbService A shared pointer to the DatabaseService
    */
   virtual void reconfigureDb(sql::DbServiceFactoryPtr dbService);

   virtual sql::DbServiceFactoryPtr dbServiceFactory();
};

/** @}*/

} // namespace web
} // namespace tbs
