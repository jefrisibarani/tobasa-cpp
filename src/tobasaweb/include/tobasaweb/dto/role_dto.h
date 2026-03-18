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

struct UserRoleDto
{
   long        id;
   long        roleId;
   bool        isPrimary;
   std::string name;
   std::string alias;
   bool        enabled;
   bool        sysRole;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserRoleDto, id, roleId, isPrimary, name, alias, enabled, sysRole)


/** @}*/

} // namespace dto
} // namespace web
} // namespace tbs
