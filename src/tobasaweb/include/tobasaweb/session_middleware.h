#pragma once

#include "tobasaweb/exp_imp.h"

#include <tobasahttp/server/common.h>
#include "tobasaweb/middleware.h"
#include "tobasaweb/cookie.h"
#include "tobasaweb/session.h"

namespace tbs {
namespace web {

/** \addtogroup WEB
 * @{
 */

/// Returns true when a request needs session processing despite the default
/// skip for AuthScheme::NONE. Earlier ignore and noSessionList rules still apply.
using SessionRequirementHandler = std::function<bool(const http::HttpContext& context)>;

/// Configures session processing, cookie handling, and session-related responses.
struct SessionMiddlewareOption
{
   /// Builds an error response when session processing cannot continue.
   http::ResultBuilder       resultBuilder;
   /// Login and logout paths used by cookie-based session handling.
   CookieAuthOption          cookieAuthOption;
   /// Requests matching this handler bypass session processing entirely.
   MiddlewareIgnoreHandler   ignoreHandler;
   /// Requests matching this handler do not use the default AuthScheme::NONE skip.
   SessionRequirementHandler forceSessionHandler;
   /// Selects a custom session cookie by request-path prefix. The first match is used.
   /// If the list is non-empty and no path matches, no session cookie is sent.
   std::vector<Cookie>       cookieRules = {} ;
   /// Overrides cookieAuthOption.loginPath for the current request.
   std::function<std::string(const http::HttpContext& context)> loginPathBuilder;
};

/// Configures session middleware options before Webapp registers the middleware.
using SessionMiddlewareOptionBuilder = std::function<void(SessionMiddlewareOption& option)>;

/**
 * Processes session cookies and server-side session state for each request.
 * Creates sessions when the configured policy requires them, checks stored
 * session expiry, and restores valid logged-in user identity into AuthResult.
 */
class TBS_WEB_API SessionMiddleware : public Middleware
{
public:
   SessionMiddleware();
   virtual ~SessionMiddleware() = default;

   virtual http::RequestStatus invoke(const http::HttpContext& context) override;
   void option(SessionMiddlewareOption option);

private: 

   SessionMiddlewareOption _option;

   /// Transform Result object into HttpContext's response
   void applyHttpResult( const http::HttpContext& context,
      http::StatusCode statusCode, const std::string& message);

   /// Skip session processing for AuthScheme::NONE,  request path '/' and login path
   bool skipSessionProcessing(const http::HttpContext& context);
   void setupNewSession(const http::HttpContext& context);
};

/** @}*/

} // namespace http
} // namespace tbs