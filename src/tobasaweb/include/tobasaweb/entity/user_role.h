#pragma once

#include <string>
#include <memory>

namespace tbs {
namespace web {
namespace entity {

/** \ingroup WEB
 * @{
 */

struct UserRole
{
   long id;
   long userId;
   long roleId;
   bool isPrimary;
};

using UserRolePtr = std::shared_ptr<UserRole>;


struct UserRoleView
{
   long        id;
   long        userId;
   std::string userName;
   long        roleId;
   std::string roleName;
};

using UserRoleViewPtr = std::shared_ptr<UserRoleView>;

/** @}*/

} // namespace entity
} // namespace web
} // namespace tbs