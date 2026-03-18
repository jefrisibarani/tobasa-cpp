#include <tobasa/util_string.h>
#include "tobasahttp/authentication.h"

namespace tbs {
namespace http {

AuthScheme authSchemeFromString(const std::string& scheme)
{
   if (!scheme.empty())
   {
      if (scheme == "bearer")
         return AuthScheme::BEARER;
      if (scheme == "basic")
         return AuthScheme::BASIC;
      if (scheme == "cookie")
         return AuthScheme::COOKIE;
      if (scheme == "custom")
         return AuthScheme::CUSTOM;
   }

   return AuthScheme::NONE;
}

std::string authSchemeToString(AuthScheme scheme)
{
   if (scheme == AuthScheme::BEARER)
      return "bearer";
   if (scheme == AuthScheme::BASIC)
      return "basic";
   if (scheme == AuthScheme::COOKIE)
      return "cookie";
   if (scheme == AuthScheme::CUSTOM)
      return "custom";      

   return "";
}

AuthContextPtr parseAuthorizationHeader(const std::string& rawText)
{
   auto context = std::make_shared<AuthContext>();

   if (rawText.empty())
   {
      context->scheme = AuthScheme::NONE;
      context->rawText = {};
   }
   else
   {
      std::string_view valueV {rawText};

      if (util::startsWith(valueV, "Basic ") /*valueV.starts_with("Basic ")*/  )
      {
         context->scheme = AuthScheme::BASIC;
         valueV.remove_prefix(6);
         context->rawText = std::string{valueV};
      }
      else if (util::startsWith(valueV, "Bearer ") /*valueV.starts_with("Bearer ")*/)
      {
         context->scheme = AuthScheme::BEARER;
         valueV.remove_prefix(7);
         context->rawText = std::string{valueV};
      }
      else if (util::startsWith(valueV, "Bearer, ")) // received via sec-websocket-protocol header
      {
         context->scheme = AuthScheme::BEARER;
         valueV.remove_prefix(8);
         context->rawText = std::string{valueV};
      }
      else
      {
         context->scheme = AuthScheme::NONE;
         context->rawText = {};
      }
   }
   return context;
}

} // namespace http
} // namespace tbs