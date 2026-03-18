#pragma once

#include <tobasaweb/result.h>

namespace tbs {
namespace web {

/** \addtogroup WEB
 * @{
 */

class MiddlewareBase;
class WebServiceBase;

/** 
 * Middleware factory base class
 */
class MiddlewareFactoryBase
{
public:
   MiddlewareFactoryBase( const MiddlewareFactoryBase & ) = delete;
   MiddlewareFactoryBase( MiddlewareFactoryBase && ) = delete;
   MiddlewareFactoryBase() = default;
   virtual ~MiddlewareFactoryBase() = default;

   virtual bool initialized() = 0;
   virtual void initMiddleware(WebServiceBase* webService) = 0;
   virtual void reconfigureDb(WebServiceBase* webService) = 0;
   virtual std::shared_ptr<MiddlewareBase> middleWare() = 0;

   void resultBuilder(http::ResultBuilder builder)
   {
      _resultBuilder = builder;
   }

   http::ResultBuilder resultBuilder()
   {
      return _resultBuilder;
   }

   virtual void middelwareName(const std::string& name) = 0;
   virtual std::string middelwareName() const = 0;

protected:
   http::ResultBuilder _resultBuilder {nullptr};
};

using MiddlewareFactoryPtr = std::shared_ptr<MiddlewareFactoryBase>;

/** @}*/

} // namespace web
} // namespace tbs