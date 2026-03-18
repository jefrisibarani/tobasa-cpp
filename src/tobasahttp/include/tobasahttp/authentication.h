#pragma once

#include <string>
#include <memory>

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/**
 * Authentication scheme
 * \sa https://www.iana.org/assignments/http-authschemes/http-authschemes.xhtml
 */
enum class AuthScheme : uint8_t
{
   NONE = 0,
   BASIC,
   BEARER,
   COOKIE,
   CUSTOM
   //DIGEST,
   //HOBA,
   //MUTUAL,
   //NEGOTIATE,
   //OAUTH,
   //SCRAM-SHA-1,
   //SCRAM-SHA-256,
   //VAPID
};

/**
 * Authentication context.
 * HTTP Context authentication properties
 */
struct AuthContext
{
   AuthContext()
   {}
   /// Authentication scheme calculated from request header.
   AuthScheme  scheme                {AuthScheme::NONE};

   /// Defined authentication scheme when registering route in Controller's route handler.
   AuthScheme  definedScheme         {AuthScheme::NONE};

   /// Effective auth scheme .
   AuthScheme  effectiveScheme       {AuthScheme::NONE};

   /// Text value from request header.
   std::string rawText               {};

   /// Authentication/authorization check will not be performed when set to true.
   bool        disableCheck          {false};

   /// Authentication header found in request header.
   bool        headerFound           {false};

   /// Marked as Ignored by AuthenticationMiddleware IgnoreHandler
   bool        authenticationIgnored {false};

   /// Marked as Ignored by AuthorizationMiddleware IgnoreHandler
   bool        authorizationIgnored  {false};
};

/// AuthContext shared pointer.
using AuthContextPtr  = std::shared_ptr<AuthContext>;

/// Convert string to AuthScheme.
AuthScheme authSchemeFromString(const std::string& scheme);

/// Convert AuthScheme to string.
std::string authSchemeToString(AuthScheme scheme);

/**
 * \brief Parse authentication from http header text.
 * \param rawText
 * \return shared pointer of AuthContext
 */
AuthContextPtr parseAuthorizationHeader(const std::string& rawText);

/** @}*/

} // namespace http
} // namespace tbs