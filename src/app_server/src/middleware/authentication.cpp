#include <tobasa/config.h>
#include <tobasa/util_string.h>
#include <tobasahttp/request.h>
#include <tobasahttp/response.h>
#include <tobasahttp/status_codes.h>
#include <tobasahttp/authentication.h>
#include <tobasaweb/credential_info.h>

#include "api_result.h"
#include "../app_util.h"
#include "../main_helper.h"

#include "authentication.h"

namespace tbs {
namespace web {


void buildAuthenticationMiddlewareOption(const conf::Webapp& webappOpt, AuthenticationMiddlewareOption& option)
{
   // Create or set a custom result builder; otherwise, the AuthenticationMiddleware
   // will use http::JsonResult or http::StatusResult by default
   /*
   option.resultBuilder =
      [](const http::HttpContext& context, http::StatusCode statusCode, const std::string message)
      {
         if (context->request()->authContext()->scheme == http::AuthScheme::BEARER || util::startsWith(context->request()->path(), "/api"))
            return web::apiResult(message, static_cast<int>(statusCode));
         else
            return http::statusResultHtml(statusCode, message);
      };
   */
   //web::CookieAuthOption cookieOpt;
   //option.cookieAuthOption = cookieOpt;
   option.cookieAuthOption.loginPath  = webappOpt.webService.loginPage;
   option.cookieAuthOption.logoutPath = webappOpt.webService.logoutPage;
   option.loginPathBuilder = app::loginPathBuilder;

   // Request paths ignored in AuthenticationMiddelware, also ignored in AuthorizationMiddleware
   option.ignoreHandler = [](const http::HttpContext& context)
      {
         auto reqPath = context->request()->path();

         bool ignore  =
            (     util::endsWith(reqPath, ".js")
               || util::endsWith(reqPath, ".css")
               || util::endsWith(reqPath, ".map")
               || util::endsWith(reqPath, ".woff2")
               || util::endsWith(reqPath, ".ttf")

               || util::startsWith(reqPath, "/assets")
               || util::startsWith(reqPath, "/css")
               || util::startsWith(reqPath, "/js")
               || util::startsWith(reqPath, "/vendor")
               || util::startsWith(reqPath, "/favicon.ico")
            );
         return ignore;
      };

   option.authHandler = [](const http::HttpContext& context, AuthenticationMiddleware& middleware)
      {
         auto response   = context->response();
         auto reqHeaders = context->request()->headers();
         auto reqPath    = context->request()->path();

         // AuthScheme::CUSTOM
         if ( reqHeaders.hasField("x-token") && reqHeaders.hasField("x-username") )
         {
            auto& authResult = std::any_cast<web::AuthResult&>( context->userData() );
            context->request()->authContext()->effectiveScheme =  http::AuthScheme::BEARER; 

            // Handle x-token and x-username field in Request Header
            // only path starts with /api
            std::string_view reqPathv {reqPath};
            if ( util::startsWith(reqPathv, "/api" ) || util::startsWith(reqPathv, "/chat_app_socket" ) )
            {
               Logger::logD("[webapp] [conn:{}] {}", context->connId(), "Handling Bearer Authentication scheme from X-token header and /api path");

               const std::string jwtToken = reqHeaders.value("x-token");
               const std::string username = reqHeaders.value("x-username");

               if (jwtToken.empty() || username.empty())
               {
                  std::string errorMsg("Invalid X-token or X-username");
                  authResult.errorMessage = errorMsg;
                  Logger::logE("[webapp] [conn:{}] {}", context->connId(), errorMsg);
                  return false;
               }
               else
               {
                  authResult.credentialsProvided = true;
                  // Verify the token and save the result into authResult
                  middleware.verifyBearerAuth(context, jwtToken, username);
                  if ( !authResult.credentialsValid && !authResult.errorMessage.empty() )
                  {
                     std::string errorMsg = "Auth JWT, "  + authResult.errorMessage;
                     authResult.errorMessage = errorMsg;
                     response->addHeader("WWW-Authenticate", R"(Bearer realm="Valid JWT should be provided", charset="utf-8")");
                     return false;
                  }
               }
            }
            else
               Logger::logD("[webapp] [conn:{}] {}", context->connId(), "X-token and X-username header found, request path invalid");
         }
         else
            Logger::logT("[webapp] [conn:{}] {}", context->connId(), "No supported Authentication header found");
         
         // AuthScheme::NONE
         return true;
      };
}


} // namespace web
} // namespace tbs