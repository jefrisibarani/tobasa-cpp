#pragma once

#include <string>
#include <memory>
#include <tobasa/datetime.h>
#include <tobasa/json.h>

namespace tbs {
namespace web {
namespace entity {

/** \ingroup WEB
 * @{
 */

struct Role
{
   long        id;
   std::string name;
   std::string alias;
   bool        enabled;
   bool        sysRole;
   DateTime    created;
   DateTime    updated;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Role, id, name, alias, enabled, sysRole)

using RolePtr  = std::shared_ptr<Role>;
using RoleList = std::vector<Role>;


struct RoleAddUser
{
   long roleId;
   std::vector<long> userIds;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RoleAddUser, roleId, userIds)

/** @}*/

} // namespace entity
} // namespace web
} // namespace tbs