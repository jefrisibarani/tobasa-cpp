#pragma once

#include <tobasahttp/server/common.h>
#include "tobasaweb/middleware.h"
#include "tobasaweb/result.h"

namespace tbs {
namespace web {

class RouterBase;

/** 
 * \ingroup WEB
 * Middleware manager and HTTP server request handler.
 */
class MiddlewareManager
{
private:
   using MiddlewareList = std::vector<MiddlewarePtr>;

   MiddlewareList  _middlewares;
   bool            _chainConfigured {false};
   std::shared_ptr<RouterBase> _router;

public:
   MiddlewareManager();
   MiddlewareManager(std::shared_ptr<RouterBase> router);
   ~MiddlewareManager();

   /// Add middleware into our list
   void use(MiddlewarePtr middleware)
   {
      _middlewares.push_back(middleware);
   }

   /// Add RequestHandlerChained into middleware list
   void use(http::RequestHandlerChained handler, const std::string& name={})
   {
      auto middleware = std::make_shared<AutoMiddleware>(handler);
      middleware->name(name);
      _middlewares.push_back(middleware);
   }

   /// HTTP server request handler, to be called by Http server instance
   http::RequestStatus invoke( const http::HttpContext& context )
   {
      return doInvoke(context);
   }

   void router(std::shared_ptr<RouterBase> router);

private:
   /// Handle http request
   http::RequestStatus doInvoke(const http::HttpContext& context);
};

} // namespace http
} // namespace tbs