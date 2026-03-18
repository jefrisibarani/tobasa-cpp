#pragma once

#include <tobasahttp/server/common.h>

namespace tbs {
namespace web {

http::RequestStatus contentTypeMiddleware(const http::HttpContext& context, const http::RequestHandler& handler);

} // namespace web
} // namespace tbs