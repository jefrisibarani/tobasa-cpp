#include <string>
#include <tobasa/config.h>
#include <tobasa/util_string.h>
#include <tobasahttp/request.h>
#include <tobasahttp/response.h>
#include <tobasahttp/status_codes.h>
#include "../matcher_regex.h"
#include "../matcher_wildcard.h"
#include "../settings_header_rules.h"
#include "cache_control.h"

namespace tbs {
namespace web {

http::RequestStatus cacheControlMiddleware(const http::HttpContext& context, const http::RequestHandler& next)
{
   auto ruleOption = Config::getOption<web::conf::HttpResponseHeaderRule>("httpResponseHeaderRule");
   const auto& req = context->request();
   auto host1       = req->headers().value("Host"); // Use Host header
   auto host        = req->authority();

   bool applied = false;
   for (const auto& entry : ruleOption.cacheControl)
   {
      if (applied) 
         break;

      // Check the host. Check for exact or match any patterns in entry.hostOrigin 
      bool hostMatch = false;
      for (const auto& pattern : entry.host)
      {
         if (/*"*"==pattern ||*/host==pattern || WildcardMatcher::instance().match(host, pattern) ) 
         {
            hostMatch = true;
            break;
         }
      }
      
      if (!hostMatch)
         continue;

      // Match request path: Check for * or match starts_with
      bool pathMatch = false;
      for (const auto& p : entry.requestPath)
      {
         if ("*"==p || util::startsWith(req->path(), p)) 
         {
            pathMatch = true;
            break;
         }
      }
      if (!pathMatch) 
         continue;

      // Match rules: Check for * or matching pattern
      for (const auto& rule  : entry.rules)
      {
         bool patternMatch = false;

         if (rule .pattern == "*") {
            patternMatch = true;
         }
         else
         {
            try 
            {
               if (RegexMatcher::instance().match(req->path(),rule.pattern)  )
                 patternMatch = true;
            }
            catch (const std::regex_error&/*e*/) {
               continue; // skip
            }
         }

         if (patternMatch)
         {
            context->response()->setHeader("Cache-Control", rule.header);
            applied = true;
            break; // stop here
         }
      }
   }

   return next(context);
}


} // namespace web
} // namespace tbs