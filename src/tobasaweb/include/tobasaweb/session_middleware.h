#pragma once

#include <tobasahttp/server/common.h>
#include "tobasaweb/middleware.h"
#include "tobasaweb/cookie.h"
#include "tobasaweb/session.h"

namespace tbs {
namespace web {

/** \addtogroup WEB
 * @{
 */

/** 
 * SessionMiddlewareOption
 */
struct SessionMiddlewareOption
{
   http::ResultBuilder resultBuilder;
   CookieAuthOption cookieAuthOption;
   MiddlewareIgnoreHandler ignoreHandler;
   std::vector<Cookie> cookieRules = {} ;
   std::function<std::string(const http::HttpContext& context)> loginPathBuilder;
};

/** 
 * SessionMiddlewareOptionBuilder
 */
using SessionMiddlewareOptionBuilder =
   std::function<void(SessionMiddlewareOption& option)>;

/** 
 * SessionMiddleware
 */
class SessionMiddleware
   : public Middleware
{
public:
   SessionMiddleware();
   virtual ~SessionMiddleware() = default;

   virtual http::RequestStatus invoke(const http::HttpContext& context);
   void option(SessionMiddlewareOption option);

private: 
   SessionMiddlewareOption _option;

   /// Transform Result object into HttpContext's response
   void applyHttpResult(
      const http::HttpContext& context,
      http::StatusCode statusCode,
      const std::string& message);   

   /// Skip session processing for AuthScheme::NONE,  request path '/' and login path
   bool skipSessionProcessing(const http::HttpContext& context);
   void setupNewSession(const http::HttpContext& context);
};

/** @}*/

} // namespace http
} // namespace tbs