#include <string>
#include <vector>
#include <tobasa/config.h>
#include <tobasa/util_string.h>
#include <tobasa/logger.h>
#include <tobasahttp/request.h>
#include <tobasahttp/response.h>
#include <tobasahttp/status_codes.h>
#include <tobasaweb/result.h>

#include "../matcher_wildcard.h"
#include "../settings_header_rules.h"
#include "response_header_rule.h"

namespace tbs {
namespace web {

namespace {
   bool isCorsHeader(const std::string& hdr)
   {
      if (    util::toLower(hdr) == "access-control-allow-origin"
           || util::toLower(hdr) == "access-control-allow-credentials"
           || util::toLower(hdr) == "access-control-allow-methods"
           || util::toLower(hdr) == "access-control-allow-headers"
           || util::toLower(hdr) == "access-control-expose-headers"
           || util::toLower(hdr) == "access-control-max-age" )
      {
         return true;
      }
      else
         return false;
   }
}

http::RequestStatus responseHeaderRuleMiddleware(const http::HttpContext& context, const http::RequestHandler& next)
{
   auto ruleOption = Config::getOption<web::conf::HttpResponseHeaderRule>("httpResponseHeaderRule");
   const auto& req = context->request();
   auto origin     = req->headers().value("Origin");  // e.g: https://localhost:8085, http://192.168.0.1:8084
   //auto host1    = req->headers().value("Host");    // e.g: localhost:8085
   auto host       = req->authority();

   if (!origin.empty()) {
      auto e=1;
   }

   // Apply rule when origin matches
   for (const auto& entry : ruleOption.headerRule)
   {
      std::string hostOrigin = host;
      // CORS requires Origin header
      if (entry.type == "cors") {
         hostOrigin = origin;
      }

      // Check the host or origin: Check for exact or match any patterns in entry.hostOrigin 
      bool hostOriginMatch=false;
      for (const auto& pattern : entry.hostOrigin)
      {
         if (/*"*"==pattern ||*/hostOrigin==pattern || WildcardMatcher::instance().match(hostOrigin, pattern) ) 
         {
            hostOriginMatch = true;
            break;
         }
      }

      if (!hostOriginMatch)
         continue;
      
      if (entry.type == "cors")
      {
         for (auto& path: entry.requestPath)
         {
            // Match request path: Check for * , / or match starts_with
            if ( "*"==path || "/"==path || util::startsWith(req->path(), path ) )
            {
               std::vector<conf::KeyValue> collectedHeaders;
               bool hasAllowOrigin            = false;
               bool hasAllowCredentials       = false;
               std::string allowOriginVal     = "";
               std::string allowCredentialVal = "";

               for (auto& hdr: entry.headers)
               {
                  if ( !isCorsHeader(hdr.key) )
                  {
                     Logger::logW("[webapp] [conn:{}] {} ", context->connId(), "Invalid header for CORS rule");
                     continue;
                  }

                  if ( util::toLower(hdr.key) == "access-control-allow-credentials") 
                  {
                     hasAllowCredentials = true;
                     allowCredentialVal = hdr.value;
                  }
                  if ( util::toLower(hdr.key) == "access-control-allow-origin" ) 
                  {
                     hasAllowOrigin = true;
                     allowOriginVal = hdr.value;
                  }
                  collectedHeaders.push_back(hdr);
               }

               if (hasAllowOrigin && hasAllowCredentials && allowCredentialVal=="true" && allowOriginVal=="*") {
                  allowCredentialVal = "false";
               }
               else if (hasAllowOrigin && allowOriginVal=="dynamic") {
                  allowOriginVal = hostOrigin;
               }

               for (auto& hdr: collectedHeaders)
               {
                  if ( util::toLower(hdr.key) == "access-control-allow-origin") 
                     context->response()->setHeader("Access-Control-Allow-Origin", allowOriginVal);
                  else if ( util::toLower(hdr.key) == "access-control-allow-credentials") 
                     context->response()->setHeader("Access-Control-Allow-Credentials", allowCredentialVal);
                  else {
                     context->response()->setHeader(hdr.key, hdr.value);
                  }
               }
            }
         }
      }
      else
      {
         for (auto& path: entry.requestPath)
         {
            // Match request path: Check for * , / or match starts_with
            if ( "*"==path || "/"==path || util::startsWith(req->path(), path ) )
            {
               for (auto& hdr: entry.headers)
               {
                  if ( isCorsHeader(hdr.key) )
                  {
                     Logger::logW("[webapp] [conn:{}] {} ", context->connId(), "Invalid header for non CORS rule");
                     continue;
                  }
                  else
                     context->response()->setHeader(hdr.key, hdr.value);
               }
            }
         }
      }
   }

   if (context->request()->method() == "OPTIONS")
   {
      auto result = http::Result();
      result.statusCode(http::StatusCode::NO_CONTENT);
      result.toResponse(context->response());
      return http::RequestStatus::handled;
   }

   return next(context);
}


} // namespace web
} // namespace tbs