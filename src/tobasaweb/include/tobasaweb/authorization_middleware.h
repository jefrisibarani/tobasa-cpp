#pragma once

#include <tobasahttp/server/common.h>
#include "tobasaweb/middleware.h"
#include "tobasaweb/cookie.h"

namespace tbs {
namespace web {

/** \addtogroup WEB
 * @{
 */

/**
 * \brief Options for the AuthorizationMiddleware.
 */
struct AuthorizationMiddlewareOption
{
   /// Functor to create Result object
   http::ResultBuilder     resultBuilder;
   CookieAuthOption        cookieAuthOption;
   MiddlewareIgnoreHandler ignoreHandler;
   std::function<std::string(const http::HttpContext& context)> loginPathBuilder;
};

/**
 * \brief Functor for configuring AuthorizationMiddlewareOption members.
 */
using AuthorizationMiddlewareOptionBuilder =
   std::function<void(AuthorizationMiddlewareOption& option)>;


/**
 * \brief Handles authorization for incoming request.
 */
class AuthorizationMiddleware
   : public Middleware
{
public:
   AuthorizationMiddleware();
   virtual ~AuthorizationMiddleware() = default;

   /**
    * \brief Invokes the authorization logic for incoming request.
    * \param context The HTTP context containing request information.
    * \return The status of the HTTP request after authorization processing.
    */
   virtual http::RequestStatus invoke(const http::HttpContext& context);
   
   /**
    * \brief Sets middleware options for the AuthorizationMiddleware.
    * \param option The option to be set
    */   
   void option(AuthorizationMiddlewareOption option);

protected:
   AuthorizationMiddlewareOption _option;

   void applyHttpResult(
      const http::HttpContext& context,
      http::StatusCode statusCode,
      const std::string& message);
};

/** @}*/

} // namespace http
} // namespace tbs