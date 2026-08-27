#pragma once

#include <tobasahttp/server/common.h>
#include <tobasaweb/session_middleware.h>
#include <tobasaweb/settings_webapp.h>

namespace tbs {
namespace web {

void builSessionMiddlewareOption(const conf::Webapp& webappOpt, SessionMiddlewareOption& option);

} // namespace web
} // namespace tbs