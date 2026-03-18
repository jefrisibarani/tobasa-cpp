#pragma once

#include <tobasahttp/server/common.h>
#include <tobasaweb/authentication_middleware.h>
#include <tobasaweb/settings_webapp.h>

namespace tbs {
namespace web {

http::RequestStatus exceptionHandlerMiddleware(const http::HttpContext& context, const http::RequestHandler& next);

} // namespace web
} // namespace tbs