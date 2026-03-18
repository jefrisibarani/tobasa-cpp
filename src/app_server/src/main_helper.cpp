#include "main_helper.h"
#include "app_util.h"

namespace tbs {
namespace app {

std::string loginPathBuilder(const http::HttpContext& httpCtx)
{
   std::string userAgent = httpCtx->request()->headers().value("User-Agent");
   if ( util::contains(userAgent, "TBSMOBILEAPP_TOBASA") )
   {
      return "/med/login";
   }

   return "/login";
}

} // app
} // tbs