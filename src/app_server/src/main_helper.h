#pragma once

#include <string>
#include <tobasahttp/server/common.h>

namespace tbs{
namespace app {

std::string loginPathBuilder(const http::HttpContext& httpCtx);


} // app
} // tbs