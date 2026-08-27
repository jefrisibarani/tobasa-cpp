#pragma once

#include <string>
#include <tobasahttp/authentication.h>

namespace tbs {
namespace web {

/**
 * \ingroup WEB
 * \brief Represents options for cookie-based authentication.
 */
struct CookieAuthOption
{
   std::string loginPath;
   std::string logoutPath;
   bool redirectToLoginPage = true;
};


} // namespace web
} // namespace tbs