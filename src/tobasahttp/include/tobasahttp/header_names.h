#pragma once

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/** 
 * HTTP Header names.
 */
enum class HeaderName : uint8_t
{
   UNKNOWN = 0,

   A_IM,
   ACCEPT,
   ACCEPT_CHARSET,
   ACCEPT_ENCODING,
   ACCEPT_LANGUAGE,
   ACCEPT_DATETIME,
   ACCESS_CONTROL_REQUEST_METHOD,
   ACCESS_CONTROL_REQUEST_HEADERS,
   AUTHORIZATION,
   CACHE_CONTROL,
   CONNECTION,
   CONTENT_LENGTH,
   CONTENT_TYPE,
   COOKIE,
   DATE,
   EXPECT,
   FORWARDED,
   FROM,
   HOST,
   IF_MATCH,
   IF_MODIFIED_SINCE,
   IF_NONE_MATCH,
   IF_RANGE,
   IF_UNMODIFIED_SINCE,
   MAX_FORWARDS,
   ORIGIN,
   PRAGMA,
   PROXY_AUTHORIZATION,
   RANGE,
   REFERER,
   TE,
   USER_AGENT,
   UPGRADE,
   VIA,
   WARNING,
   SEC_WEBSOCKET_PROTOCOL
};

/** 
 * \brief Get HeaderName as string.
 * \param code
 * \return std::string
 */
inline std::string headerNameToString(HeaderName code)
{
   switch(code)
   {
      case HeaderName::A_IM:
         return "a-im";
      case HeaderName::ACCEPT:
         return "accept";
      case HeaderName::ACCEPT_CHARSET:
         return "accept-charset";
      case HeaderName::ACCEPT_ENCODING:
         return "accept-encoding";
      case HeaderName::ACCEPT_LANGUAGE:
         return "accept-language";
      case HeaderName::ACCEPT_DATETIME:
         return "accept-datetime";
      case HeaderName::ACCESS_CONTROL_REQUEST_METHOD:
         return "access-control-request-method";
      case HeaderName::ACCESS_CONTROL_REQUEST_HEADERS:
         return "access-control-request-headers";
      case HeaderName::AUTHORIZATION:
         return "authorization";
      case HeaderName::CACHE_CONTROL:
         return "cache-control";
      case HeaderName::CONNECTION:
         return "connection";
      case HeaderName::CONTENT_LENGTH:
         return "content-length";
      case HeaderName::CONTENT_TYPE:
         return "content-type";
      case HeaderName::COOKIE:
         return "cookie";
      case HeaderName::DATE:
         return "date";
      case HeaderName::EXPECT:
         return "expect";
      case HeaderName::FORWARDED:
         return "forwarded";
      case HeaderName::FROM:
         return "from";
      case HeaderName::HOST:
         return "host";
      case HeaderName::IF_MATCH:
         return "if-match";
      case HeaderName::IF_MODIFIED_SINCE:
         return "if-modified-since";
      case HeaderName::IF_NONE_MATCH:
         return "if-none-match";
      case HeaderName::IF_RANGE:
         return "if-range";
      case HeaderName::IF_UNMODIFIED_SINCE:
         return "if-unmodified-since";
      case HeaderName::MAX_FORWARDS:
         return "max-forwards";
      case HeaderName::ORIGIN:
         return "origin";
      case HeaderName::PRAGMA:
         return "pragma";
      case HeaderName::PROXY_AUTHORIZATION:
         return "proxy-authorization";
      case HeaderName::RANGE:
         return "range";
      case HeaderName::REFERER:
         return "referer";
      case HeaderName::TE:
         return "te";
      case HeaderName::USER_AGENT:
         return "user-agent";
      case HeaderName::UPGRADE:
         return "upgrade";
      case HeaderName::VIA:
         return "via";
      case HeaderName::WARNING:
         return "warning";
      case HeaderName::SEC_WEBSOCKET_PROTOCOL:
         return "sec-websocket-protocol";
   }

   return "unknown";
}

/** @}*/

} // namespace http
} // namespace tbs