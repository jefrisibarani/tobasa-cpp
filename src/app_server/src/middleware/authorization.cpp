#include <tobasa/config.h>
#include <tobasa/util_string.h>
#include <tobasahttp/request.h>
#include <tobasahttp/response.h>
#include <tobasahttp/status_codes.h>
#include "api_result.h"
#include "../app_util.h"
#include "../main_helper.h"
#include "authorization.h"

namespace tbs {
namespace web {

void builAuthorizationMiddlewareOption(const conf::Webapp& webappOpt, AuthorizationMiddlewareOption& option)
{
   // Create or set a custom result builder; otherwise, the AuthorizationMiddleware
   // will use http::JsonResult or http::StatusResult by default
   /*
   option.resultBuilder =
      [](const http::HttpContext& context, http::StatusCode statusCode, const std::string message)
      {
         if (util::startsWith(context->request()->path(), "/api"))
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

   option.ignoreHandler = [](const http::HttpContext& context)
      {
         auto reqPath = context->request()->path();
         bool ignore  =
            (     util::endsWith(reqPath, ".js")
               || util::endsWith(reqPath, ".css")
               || util::endsWith(reqPath, ".map")
               || util::startsWith(reqPath, "/favicon.ico")
            );
         return ignore;
      };
}


} // namespace web
} // namespace tbs