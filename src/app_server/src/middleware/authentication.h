#pragma once

#include <tobasahttp/server/common.h>
#include <tobasaweb/authentication_middleware.h>
#include <tobasaweb/settings_webapp.h>

namespace tbs {
namespace web {

void buildAuthenticationMiddlewareOption(const conf::Webapp& webappOpt, AuthenticationMiddlewareOption& option);

} // namespace web
} // namespace tbs