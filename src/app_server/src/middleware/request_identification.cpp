#include <tobasa/config.h>
#include <tobasa/util_string.h>
#include <tobasahttp/request.h>
#include <tobasahttp/response.h>
#include <tobasahttp/status_codes.h>
#include "api_result.h"
#include "../app_util.h"
#include "request_identification.h"

namespace tbs {
namespace web {

http::RequestStatus requestIdentificationMiddleware(const http::HttpContext& context, const http::RequestHandler& next)
{
   auto& headers = context->request()->headers();

   auto userAgent           = headers.value("User-Agent");
   auto projectId           = headers.value("X-Project-Id");
   auto clientAppId         = headers.value("X-Client-App-Id");
   auto webserviceVersion   = headers.value("X-WebService-Version");
   auto deviceToken         = headers.value("X-Device-Token");
   auto chatProtocolVersion = headers.value("X-ChatProto-Version");

   if ( (    context->request()->path() == "/chat_app_socket" 
          || util::startsWith(context->request()->path(), "/api/chat")
          || util::startsWith(context->request()->path(), "/chat/") )
         && !app::isValidAppClientId(clientAppId)
      )
   {
      //web::ApiResult result("Invalid client app id", 400, http::StatusCode::BAD_REQUEST);
      //result.toResponse(context->response());
      //return http::RequestStatus::handled;
   }

   if (util::contains(userAgent, "TBSMOBILEAPP_TOBASA")) {
   }

   return next(context);
}


} // namespace web
} // namespace tbs