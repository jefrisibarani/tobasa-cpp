#pragma once

#include <tobasahttp/server/common.h>

namespace tbs {
namespace web {

http::RequestStatus requestIdentificationMiddleware(const http::HttpContext& context, const http::RequestHandler& next);

} // namespace web
} // namespace tbs