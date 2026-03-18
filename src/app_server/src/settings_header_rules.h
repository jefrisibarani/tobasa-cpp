#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace tbs {
namespace web {
namespace conf {

/** \addtogroup WEB
 * @{
 */

struct KeyValue
{
   std::string key;     // http header key
   std::string value;   // http header value
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(KeyValue, key, value)

// HTTP header rule data, used as apply HTTP headers or response or CORS headers
struct HeaderRuleItem
{
   std::string type;                      // cors or none
   std::vector<std::string> hostOrigin;   // http request host or origin. if type is cors, this is origin, elase host
   std::vector<std::string> requestPath;  //
   std::vector<KeyValue> headers;         //
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HeaderRuleItem, type, hostOrigin, requestPath, headers)


struct PatternHeader
{
   std::string pattern; // eg: "\\.(html|htm)$""
   std::string header;  // eg: "no-cache"   ,  "public, max-age=31536000, immutable"
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PatternHeader, pattern, header)

struct CacheControlItem
{
   std::vector<std::string> host;        // http request host 
   std::vector<std::string> requestPath;
   std::vector<PatternHeader> rules;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CacheControlItem, host, requestPath, rules)

struct HttpResponseHeaderRule
{
   std::vector<CacheControlItem> cacheControl;
   std::vector<HeaderRuleItem> headerRule;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HttpResponseHeaderRule, cacheControl, headerRule)

/** @}*/

} // namespace conf
} // namespace web
} // namespace tbs
