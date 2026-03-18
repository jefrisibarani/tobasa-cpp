#pragma once

#include <tobasahttp/server/common.h>
#include "tobasaweb/middleware_factory.h"
#include "tobasaweb/service_client_base.h"

namespace tbs {
namespace web {

/** \addtogroup WEB
 * @{
 */

/**
 * \brief Base class for middleware handling HTTP server requests.
 * MiddlewareBase serves as the foundational class for middleware components,
 * which act as HTTP server request handlers. 
 * Instances of this class are instantiated by a Factory.
 */
class MiddlewareBase
   : public ServiceClientBase
{
public:
   MiddlewareBase() = default;
   virtual ~MiddlewareBase() = default;

private:
   //! Factory to manage this middleware
   template<class MiddlewareImpl> friend class MiddlewareFactory;
   template<class RouterImpl> friend class RouterFactory;

public:
   /**
    * \brief Processes an HTTP request.
    * This method, invoked by a Factory to create a functor in getHandler(),
    * handles the processing of an HTTP request within a middleware component.
    * \param context The HTTP context containing request information.
    * \return The status of processing the HTTP request.
    */
   virtual http::RequestStatus invoke(const http::HttpContext& context) = 0;

   virtual http::RequestHandler getHandler() = 0;

   virtual void nextHandler(http::RequestHandler handler) = 0;

   virtual http::RequestStatus next(const http::HttpContext& context) = 0;

   void name(const std::string& v) { _name = v; }
   std::string name() const { return _name; }

protected:

   void resultBuilder(http::ResultBuilder builder)
   {
      _resultBuilder = builder;
   }

   http::ResultBuilder _resultBuilder {nullptr};
   std::string _name;
};

using MiddlewarePtr = std::shared_ptr<MiddlewareBase>;

/** @}*/

} // namespace web
} // namespace tbs