#include <tobasa/config.h>
#include <tobasa/util_string.h>
#include <tobasahttp/request.h>
#include <tobasahttp/response.h>
#include <tobasahttp/status_codes.h>
#include "api_result.h"
#include "../app_util.h"
#include "content_type.h"

namespace tbs {
namespace web {

http::RequestStatus contentTypeMiddleware(const http::HttpContext& context, const http::RequestHandler& next)
{
   //Logger::logT("[webapp] [conn:{}] {} {}", context->connId(), "Invoking content-type handler:", context->request()->path() );
   //context->response()->addHeader("X-Processed-By",  "Content Type Middleware");
   return next(context);
}


} // namespace web
} // namespace tbs