#include <tobasa/config.h>
#include <tobasa/util_string.h>
#include <tobasahttp/request.h>
#include <tobasahttp/response.h>
#include <tobasahttp/status_codes.h>

#include "api_result.h"
#include "../app_util.h"
#include "../main_helper.h"

#include "session.h"

namespace tbs {
namespace web {

//SessionMiddlewareOptionBuilder
void builSessionMiddlewareOption(const conf::Webapp& webappOpt, SessionMiddlewareOption& option)
{
   option.cookieAuthOption.loginPath  = webappOpt.webService.loginPage;
   option.cookieAuthOption.logoutPath = webappOpt.webService.logoutPage;
   option.loginPathBuilder = app::loginPathBuilder;

   //option.cookieAuthOption.redirectToLoginPage = true;   // default value is true
   //option.cookieRule.push_back(web::CookieRule("/med", "Session"));  // no need

   // Create or set a custom result builder; otherwise, the SessionMiddlewareOption
   // will use http::JsonResult or http::StatusResult by default
   option.resultBuilder =
      [](const http::HttpContext& context, http::StatusCode statusCode, const std::string message)
      {
         if (context->request()->authContext()->scheme == http::AuthScheme::BEARER || util::startsWith(context->request()->path(), "/api"))
            return web::apiResult(message, static_cast<int>(statusCode));
         else
            return http::statusResultHtml(statusCode, message);
      };

   // The session will not be applied to paths that are not listed here.
   // You can also disable the session through the configuration file by adding the paths to the noSessionList
   // Ignoring the request path does not imply that Authentication and Authorization will be bypassed.
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
               
               || util::startsWith(reqPath, "/api")
               || util::startsWith(reqPath, "/chat_app_socket")
            );
         return ignore;
      };
}


} // namespace web
} // namespace tbs