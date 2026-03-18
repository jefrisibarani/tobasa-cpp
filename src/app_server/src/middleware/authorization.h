#pragma once

#include <tobasahttp/server/common.h>
#include <tobasaweb/authorization_middleware.h>
#include <tobasaweb/settings_webapp.h>

namespace tbs {
namespace web {


void builAuthorizationMiddlewareOption(const conf::Webapp& webappOpt, AuthorizationMiddlewareOption& option);

} // namespace web
} // namespace tbs