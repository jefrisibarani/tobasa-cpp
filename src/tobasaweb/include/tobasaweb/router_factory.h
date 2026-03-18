#pragma once

#include "tobasaweb/web_service_base.h"
#include "tobasaweb/router_base.h"
#include "tobasaweb/router_factory_base.h"

namespace tbs {
namespace web {

/** \addtogroup WEB
 * @{
 */

/** 
 * Router factory
 * @tparam RouterImpl
 */
template <typename RouterImpl>
class RouterFactory
   : public RouterFactoryBase
{
protected:
   std::shared_ptr<RouterImpl> _router;
   bool _initialized = false;

public:
   RouterFactory(const RouterFactory &) = delete;
   RouterFactory(RouterFactory &&) = delete;
   RouterFactory()
   {
      _router = std::make_shared<RouterImpl>();
   }

   virtual ~RouterFactory()
   {
      Logger::logT("[webapp] RouterFactory destroyed");
   }

   std::shared_ptr<MiddlewareBase> middleWare()
   {
      return _router;
   }

   std::shared_ptr<RouterBase> router()
   {
      return _router;
   }

   virtual bool initialized()
   {
      return _initialized;
   }

   virtual void initRouter(WebServiceBase* webService)
   {
      if ( !_initialized )
      {
         _router->setDbServiceFactory( webService->dbServiceFactory() );

         // _pRouter is a RouterImpl
         // RouterFactory<RouterImpl> is a friend of RouterBase, NOT RouterImpl
         // we need RouterBase pointer, thus allowing RouterFactory<RouterImpl>
         // to call RouterBase::onInit, whichs is a protected method
         auto basePtr = std::static_pointer_cast<RouterBase>(_router);
         basePtr->onInit();

         _initialized = true;
      }
   }

   virtual void reconfigureDb(WebServiceBase* webService)
   {
      _router->setDbServiceFactory( webService->dbServiceFactory() );
   } 

   virtual void middelwareName(const std::string& name) {}
   virtual std::string middelwareName() const { return ""; }
};

template <class RouterType>
[[nodiscard]]
RouterFactoryPtr makeRouter()
{
   return std::make_shared<tbs::web::RouterFactory<RouterType>>();
}

/** @}*/

} // namespace web
} // namespace tbs
