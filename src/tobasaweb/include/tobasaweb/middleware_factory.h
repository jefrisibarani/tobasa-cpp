#pragma once

#include "tobasaweb/web_service_base.h"
#include "tobasaweb/middleware_factory_base.h"

namespace tbs {
namespace web {

/** \addtogroup WEB
 * @{
 */

/** 
 * Middleware factory
 */
template < typename MiddlewareImpl >
class MiddlewareFactory
   : public MiddlewareFactoryBase
{
protected:
   std::shared_ptr<MiddlewareImpl> _middleware;
   bool _initialized = false;

public:
   MiddlewareFactory(const MiddlewareFactory &) = delete;
   MiddlewareFactory(MiddlewareFactory &&) = delete;
   virtual ~MiddlewareFactory() = default;

   MiddlewareFactory()
   {
      _middleware = std::make_shared<MiddlewareImpl>();
   }

   template<typename... Params>
   MiddlewareFactory(Params &&... params)
   {
      _middleware = std::make_shared<MiddlewareImpl>(std::forward<Params>(params)...);
   }

   virtual bool initialized()
   {
      return _initialized;
   }

   virtual void initMiddleware(WebServiceBase* webService)
   {
      if ( ! _initialized )
      {
         _middleware->setDbServiceFactory( webService->dbServiceFactory() );
         _middleware->resultBuilder( this->resultBuilder() );

         _initialized = true;
      }
   }

   virtual void reconfigureDb(WebServiceBase* webService)
   {
      _middleware->setDbServiceFactory( webService->dbServiceFactory() );
   }

   std::shared_ptr<MiddlewareBase> middleWare()
   {
      return _middleware;
   }

   virtual void middelwareName(const std::string& name) { _middleware->name(name); }
   virtual std::string middelwareName() const { return _middleware->name(); }
};


template <class MiddlewareType>
[[nodiscard]]
MiddlewareFactoryPtr makeMiddleware()
{
   return std::make_shared<tbs::web::MiddlewareFactory<MiddlewareType>>();
}

template <class MiddlewareType, typename... Params>
[[nodiscard]]
MiddlewareFactoryPtr makeMiddleware(Params&&... args)
{
   return std::make_shared<tbs::web::MiddlewareFactory<MiddlewareType>>(std::forward<Params>(args)...);
}

/** @}*/

} // namespace web
} // namespace tbs