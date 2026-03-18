#pragma once

#include "tobasaweb/middleware_factory_base.h"

namespace tbs {
namespace web {

/** \addtogroup WEB
 * @{
 */

class WebServiceBase;
class RouterBase;

/** 
 * Router factory base class
 * WebService does not put RouterFactoryPtr in its MiddlewareFactoryPtr collection
 * see WebService::useRouter()
 */
class RouterFactoryBase
   : public MiddlewareFactoryBase
{
public:
   RouterFactoryBase(const RouterFactoryBase &) = delete;
   RouterFactoryBase(RouterFactoryBase &&) = delete;
   RouterFactoryBase() = default;
   virtual ~RouterFactoryBase() = default;

   virtual std::shared_ptr<RouterBase> router() = 0;
   virtual void initRouter(WebServiceBase* webService) = 0;
   
   // Note: Router is a special middleware.
   // WebService will only call initRouter()
   virtual void initMiddleware(WebServiceBase* webService) {}

   virtual void reconfigureDb(WebServiceBase* webService) = 0;
};

using RouterFactoryPtr = std::shared_ptr<RouterFactoryBase>;

/** @}*/

} // namespace web
} // namespace tbs