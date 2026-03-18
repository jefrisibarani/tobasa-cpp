#include <tobasa/config.h>
#include <tobasa/util_string.h>
#include <tobasahttp/request.h>
#include <tobasahttp/response.h>
#include <tobasahttp/status_codes.h>

#include "api_result.h"
#include "../app_util.h"


#include "database_check.h"

namespace tbs {
namespace web {

http::RequestStatus databaseCheckMiddleware(
   Webapp &app, app::DbServicePtr dbService,
   const http::HttpContext& context, const http::RequestHandler& next)
{
   auto authContext = context->request()->authContext();
   if (authContext->disableCheck || authContext->effectiveScheme == http::AuthScheme::NONE)
      return next(context);

   Logger::logT("[webapp] [conn:{}] {} {}", context->connId(), "Invoking db connectivity check handler:", context->request()->path() );

   bool connectionBad = false;
   if (!app.dbConnected())
   {
      /*
      // pass new DbServiceFactoryApp instance to replace current App's db service
      auto db = std::make_shared<app::DbServiceFactoryApp>(webappOpt.dbConnection);
      if (app.reconnectDb(db))
         return next(context);
      */

      // calling reconnectDb() without passing new DbServiceFactoryApp instance, will only
      // reconnect db service
      Logger::logI("No connection to database, reconnecting...");
      if (app.reconnectDb())
         return next(context);
      else
         connectionBad = true;
   }
   else
   {
      try
      {
         if (dbService->testConnection())
            return next(context);
         else
            connectionBad = true;
      }
      catch(const tbs::SqlException& ex)
      {
         Logger::logE("{}", ex.what());
         Logger::logI("Bad DbServiceFactoryApp instance, reconnecting...");
         if (app.reconnectDb())
            return next(context);
         else
            connectionBad = true;
      }
   }

   if (connectionBad)
   {
      Logger::logE("Could not initiate connection to database, giving up");

      if (util::startsWith(context->request()->path(), "/api"))
      {
         web::ApiResult result("No connection to database", 500, http::StatusCode::INTERNAL_SERVER_ERROR);
         result.toResponse(context->response());
         return http::RequestStatus::handled;
      }
      else
      {
         http::StatusResult result(http::StatusCode::INTERNAL_SERVER_ERROR, "No connection to database");
         result.toResponse(context->response());
         return http::RequestStatus::handled;
      }
   }

   return next(context);
}


} // namespace web
} // namespace tbs