#include "tobasaweb/router.h"
#include <tobasasql/database_service_factory_base.h>
#include "tobasaweb/middleware_manager.h"
#include "tobasaweb/web_service_base.h"
#include "tobasaweb/controller_factory.h"
#include "tobasaweb/router_factory.h"
#include "tobasaweb/web_service.h"

namespace tbs {
namespace web {

WebService::WebService()
   : WebServiceBase()
{
   // create default router
   _routerFactory = std::make_shared<web::RouterFactory<web::Router>>();
   _srvhandler.router(_routerFactory->router());
}

WebService::~WebService()
{
   Logger::logT("[webapp] WebService destroyed");
}

RouterPtr WebService::router()
{
   return _routerFactory->router();
}

asio::io_context& WebService::ioContext()
{
   return *_pIoContext;
}

void WebService::useDbService(sql::DbServiceFactoryPtr service)
{
   _dbService = service;
}

void WebService::useRouter(RouterFactoryPtr factory)
{
   _routerFactory = factory;
   _srvhandler.router(_routerFactory->router());
}

void WebService::addController(ControllerFactoryPtr factory)
{
   _controllerFactories.push_back(factory);
}

void WebService::addMiddleware(MiddlewareFactoryPtr factory)
{
   _middlewareFactories.push_back(factory);
}

void WebService::addMiddleware(http::RequestHandlerChained handler, const std::string& name)
{
   auto factory = std::make_shared<web::MiddlewareFactory<web::AutoMiddleware>>(handler);
   factory->middelwareName(name);
   
   _middlewareFactories.push_back(factory);
}

void WebService::setupHandlers()
{
   for (auto factory: _middlewareFactories)
   {
      // Initialize and integrate middleware
      factory->initMiddleware(this);
      // Add middleware to the middleware manager, which serves as a request handler manager
      _srvhandler.use(factory->middleWare());
   }

   // Initialize controllers
   for (auto factory: _controllerFactories)
   {
      factory->initController(this);
   }

   // Initialize and integrate the router
   _routerFactory->initRouter(this);
   // The router also functions as middleware and a request handler.
   // Add the router to the middleware manager, which serves as a request handler manager.
   _srvhandler.use(_routerFactory->middleWare());
}

std::unique_ptr<tbs::http::RequestHandler> WebService::serverHttpRequestHandler()
{
   using namespace std::placeholders;

   auto handler = std::make_unique<tbs::http::RequestHandler>(
                     std::bind(&web::MiddlewareManager::invoke, &_srvhandler, _1)
                  );

   return std::move(handler);
}

void WebService::serverStatusPageBuilder(http::StatusPageBuilder renderer)
{
   _statusPageBuilder = renderer;
}

http::StatusPageBuilder WebService::serverStatusPageBuilder()
{
   return _statusPageBuilder;
}

void WebService::setIoContext(asio::io_context& ioContext)
{
   _pIoContext = &ioContext;
}

void WebService::reconfigureDb(sql::DbServiceFactoryPtr dbService)
{
   _dbService   = dbService;

   for (auto factory: _middlewareFactories)
   {
      factory->reconfigureDb(this);
   }

   for (auto factory: _controllerFactories)
   {
      factory->reconfigureDb(this);
   }

   _routerFactory->reconfigureDb(this);
}

sql::DbServiceFactoryPtr WebService::dbServiceFactory()
{
   return _dbService;
}


} // namespace web
} // namespace tbs
