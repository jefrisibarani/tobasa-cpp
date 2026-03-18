#pragma once

#include <tobasahttp/server/common.h>
#include <tobasaweb/webapp.h>
#include "../database_service_factory_app.h"

namespace tbs {
namespace web {

http::RequestStatus databaseCheckMiddleware(
   Webapp &app, app::DbServicePtr dbService,
   const http::HttpContext& context, const http::RequestHandler& next);

} // namespace web
} // namespace tbs