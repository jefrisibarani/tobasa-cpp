#pragma once

#include <string>
#include <vector>
#include <tobasa/json.h>

namespace tbs {
namespace web {
namespace dto {

/** \ingroup WEB
 * @{
 */

struct LoginUser
{
   std::string userName;
   std::string password;
   int         selectedSiteId = 1;
   std::string selectedLangId = "id-ID";
   std::string uniqueCode     = "";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LoginUser, userName, password, selectedSiteId, selectedLangId, uniqueCode)


struct LoginUserSimple
{
   std::string userName;
   std::string password;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LoginUserSimple, userName, password)


/** @}*/

} // namespace dto
} // namespace web
} // namespace tbs