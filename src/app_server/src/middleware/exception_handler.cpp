#include <tobasa/config.h>
#include <tobasa/json.h>
#include <tobasa/util_string.h>
#include <tobasahttp/request.h>
#include <tobasahttp/response.h>
#include <tobasahttp/status_codes.h>
#include <tobasaweb/exception.h>
#include "api_result.h"
#include "../app_util.h"
#include "../main_helper.h"
#include "exception_handler.h"

namespace tbs {
namespace web {

http::RequestStatus exceptionHandlerMiddleware(const http::HttpContext& context, const http::RequestHandler& next)
{
   bool validationErrorOccurred = false;
   bool errorOccured = false;
   std::string errMsg;
   http::StatusCode statusCode;

   try
   {
      return next(context);
   }
   catch (const tbs::ValidationException& ex)
   {
      statusCode = http::StatusCode::OK;
      validationErrorOccurred = true;
      errorOccured = true;
      errMsg = ex.what();
   }
   catch (const SqlException& ex)
   {
      statusCode = http::StatusCode::INTERNAL_SERVER_ERROR;
      errorOccured = true;
      errMsg = ex.what();
   }
   catch (const tbs::AppException& ex)
   {
      statusCode = http::StatusCode::INTERNAL_SERVER_ERROR;
      errorOccured = true;
      errMsg = ex.what();
   }
   catch (const Json::exception& ex)
   {
      statusCode = http::StatusCode::BAD_REQUEST;
      errorOccured = true;
      errMsg = cleanJsonException(ex);
   }
   catch (const std::exception& ex)
   {
      statusCode = http::StatusCode::INTERNAL_SERVER_ERROR;
      errorOccured = true;
      errMsg = ex.what();
   }

   Logger::logE("[webapp] [conn:{}] Exception caught: {}", context->connId(), errMsg);

   if (statusCode == http::StatusCode::INTERNAL_SERVER_ERROR)
   {
      // As this is the default exception handler for all routes, replace errMsg
      // with a generic message to prevent sensitive information from being displayed to the user.
      errMsg = "An error occurred while processing your request";
   }

   if (util::startsWith(context->request()->path(), "/api") ||
       util::startsWith(context->request()->contentType(), "application/json") ||
       (context->request()->headers().value("X-Request-Initiator") == "XMLHttpRequest" ) )
   {
      int resultCode = static_cast<int>(statusCode);
      if (validationErrorOccurred)
         resultCode = 202;

      // apply http response
      web::ApiResult result(errMsg, resultCode, statusCode);
      result.toResponse(context->response());
   }
   else
   {
      auto result = http::statusResultHtml(statusCode, errMsg);
      // apply http response, with custom ResultContentBuilder
      result->toResponse(context->response(),
         [](std::shared_ptr<http::Result> result)
         {
            return app::renderStatusPage(result);
         });
   }

   return http::RequestStatus::handled;
}


} // namespace web
} // namespace tbs