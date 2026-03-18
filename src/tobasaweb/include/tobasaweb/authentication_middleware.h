#pragma once

#include <tobasahttp/server/common.h>
#include <tobasahttp/authentication.h>
#include "tobasaweb/middleware.h"
#include "tobasaweb/cookie.h"

namespace tbs {
namespace web {

/** \addtogroup WEB
 * @{
 */

class AuthenticationMiddleware;


/**
 * \brief Options for the AuthenticationMiddleware.
 */
struct AuthenticationMiddlewareOption
{
   /// Functor to create Result object
   http::ResultBuilder     resultBuilder;
   CookieAuthOption        cookieAuthOption;
   MiddlewareIgnoreHandler ignoreHandler;
   std::function<std::string(const http::HttpContext& context)> loginPathBuilder;
   std::function<bool(const http::HttpContext& context, AuthenticationMiddleware& middleware)> authHandler;
};

/**
 * \brief Functor for configuring AuthenticationMiddlewareOption members.
 */
using AuthenticationMiddlewareOptionBuilder =
   std::function<void(AuthenticationMiddlewareOption& option)>;


/**
 * \brief Handles authentication for incoming request.
 * Manages the authentication process for incoming request,
 * providing functionalities to validate and authenticate user credentials or access tokens.
 */
class AuthenticationMiddleware
   : public Middleware
{
public:
   AuthenticationMiddleware();
   virtual ~AuthenticationMiddleware() = default;

   /**
    * \brief Invokes the authentication logic for incoming request.
    * \param context The HTTP context containing request information.
    * \return The status of the HTTP request after authentication processing.
    */
   virtual http::RequestStatus invoke(const http::HttpContext& context);
   
   /**
    * \brief Sets middleware options for the AuthenticationMiddleware.
    * \param option The option to be set
    */
   void option(AuthenticationMiddlewareOption option);

   /**
    * \brief Verifies the Basic authentication credentials within the provided HTTP context
    * by validating it against the given username and password.
    * \param context The HTTP context containing request information.
    * \param userName The username for Basic authentication.
    * \param password The password associated with the username (optional).
    */
   void verifyBasicAuth(
            const http::HttpContext& context,
            const std::string& userName,
            const std::string& password="");

   /**
    * \brief Verifies the Bearer authentication token within the provided HTTP context
    * by validating it against the given token and optionally a username.
    * \param context The HTTP context containing request information.
    * \param token The Bearer authentication token to be verified.
    * \param username The username associated with the token (optional).
    */
   void verifyBearerAuth(
            const http::HttpContext& context,
            const std::string& token,
            const std::string& username="");

private:

   AuthenticationMiddlewareOption _option;

   /// Transform Result object into HttpContext's response.
   void applyHttpResult(
      const http::HttpContext& context,
      http::StatusCode statusCode,
      const std::string& message);

   /**
    * \brief Represents the result of authentication parsing.
    */
   struct AuthParseResult
   {
      bool valid               = false;
      std::string username     = {};
      std::string password     = {};
      std::string errorMessage = {};
   };

   /**
    * \brief Parses and extracts basic authentication credentials from the provided authorization header.
    * \param authHeader The authorization header containing basic authentication credentials.
    * \return An AuthParseResult
    */
   AuthParseResult parseBasicAuth(const std::string& authHeader);
};

/** @}*/

} // namespace http
} // namespace tbs