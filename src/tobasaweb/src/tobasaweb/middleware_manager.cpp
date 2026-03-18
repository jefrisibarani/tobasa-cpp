#include "tobasaweb/router_base.h"
#include "tobasaweb/middleware_manager.h"

namespace tbs {
namespace web {

MiddlewareManager::MiddlewareManager() {}
MiddlewareManager::MiddlewareManager(std::shared_ptr<RouterBase> router)
{
   _router = router;
}

MiddlewareManager::~MiddlewareManager()
{
   Logger::logT("[webapp] MiddlewareManager destroyed");
}

void MiddlewareManager::router(std::shared_ptr<RouterBase> router)
{
   _router = router;
}

//! Handle http request
http::RequestStatus MiddlewareManager::doInvoke(const http::HttpContext& context)
{
   if (_middlewares.empty())
   {
      // When no responder is registered, the default request handler will be executed
      http::RequestHandler defaulthandler =
         [](const http::HttpContext& context)
         {
            Logger::logT("[webapp] MiddlewareManager executing default handler");
            http::StatusResult result(http::StatusCode::NOT_FOUND);
            result.toResponse(context->response());

            return http::RequestStatus::handled;
         };

      return (defaulthandler)(context);
   }
   else
   {
      if (!_chainConfigured)
      {
         // Create and setup chain of handlers.
         // Setup nextHandler for each RequestResponder.
         // After RequestResponder finishes processing the HTTP request, 
         // if further processing is required, it should call its nextHandler.
         // The last(only) RequestResponder in chain will not have nextHandler.
         size_t i     = 0;
         size_t size  = _middlewares.size();
         for (auto it = _middlewares.begin(); it != _middlewares.end(); ++it)
         {
            Logger::logT("[webapp] MiddlewareManager configuring {}", (*it)->name());
            ++i;
            if (i<size)
            {
               auto handler = (*it)->getHandler();         // get current responder
               auto next = std::next(it, 1);               // get next responder and its handler
               auto nextHandler = (*next)->getHandler();
               (*it)->nextHandler(std::move(nextHandler)); // assign next's handler to current responder
            }
         }
         _chainConfigured = true;
      }

      // setup request handler id, request/route authentication scheme
      _router->setupRoute(context);

      // get the first RequestResponder in the chain, and run its handler.
      auto it      = _middlewares.begin();
      auto handler = (*it)->getHandler();
      auto status  = (handler)(context);

      return status;
   }
}


} // namespace web
} // namespace tbs