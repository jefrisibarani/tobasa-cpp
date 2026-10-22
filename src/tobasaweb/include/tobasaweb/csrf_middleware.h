#pragma once

#include "tobasaweb/exp_imp.h"
#include <string>
#include <tobasahttp/server/common.h>
#include "tobasaweb/middleware.h"

namespace tbs {
namespace web {

/// Returns true when the request must provide a CSRF token.
using CsrfProtectionHandler = std::function<bool(const http::HttpContext& context)>;

/// Configures request selection, token transport, and validation failure responses.
struct CsrfMiddlewareOption
{
   /// Builds the response returned when CSRF validation fails.
   http::ResultBuilder     resultBuilder;
   /// Requests matching this handler bypass CSRF processing.
   MiddlewareIgnoreHandler ignoreHandler;
   /// Only requests matching this handler require a token; an empty handler protects all routes.
   CsrfProtectionHandler   protectHandler;
   /// Form field used to submit the token.
   std::string tokenFieldName  {"_csrf"};
   /// Request header used to submit the token.
   std::string tokenHeaderName {"X-CSRF-Token"};
   /// Session key used to store the token.
   std::string sessionDataKey  {"csrf_token"};
};

/// Configures CSRF middleware options before Webapp registers the middleware.
using CsrfMiddlewareOptionBuilder = std::function<void(CsrfMiddlewareOption& option)>;

/**
 * Validates session-bound CSRF tokens for requests selected by the options.
 *
 * On GET, HEAD, and OPTIONS requests, creates a token when a loaded session
 * does not already contain one. The application must expose that token in
 * forms or request headers. Other methods must submit the matching token in
 * the configured header or form field; missing sessions or invalid tokens
 * receive HTTP 403. Header tokens take precedence over form tokens.
 */
class TBS_WEB_API CsrfMiddleware : public Middleware
{
public:
   CsrfMiddleware();
   virtual ~CsrfMiddleware() = default;

   http::RequestStatus invoke(const http::HttpContext& context) override;
   void option(CsrfMiddlewareOption option);

private:
   CsrfMiddlewareOption _option;

   void applyHttpResult(const http::HttpContext& context,
      http::StatusCode statusCode, const std::string& message);
};

} // namespace web
} // namespace tbs