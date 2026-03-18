#pragma once

#include <tobasahttp/server/common.h>

namespace tbs {
namespace web {

http::RequestStatus cacheControlMiddleware(const http::HttpContext& context, const http::RequestHandler& handler);

} // namespace web
} // namespace tbs