#pragma once

#include <iostream>
#include <vector>
#include <tuple>
#include <tobasasql/sql_service_base.h>
#include <tobasasql/sql_query_option.h>
#include "tobasaweb/entity/user.h"
#include "tobasaweb/entity/role.h"
#include "tobasaweb/entity/menu.h"
#include "tobasaweb/entity/acl.h"

namespace tbs {
namespace app {

/** \addtogroup APP 
 * @{
 */

/** 
 * UserAclDbRepoBase.
 */
class UserAclDbRepoBase : public sql::SqlServiceBase
{
public:
   UserAclDbRepoBase() = default;
   virtual ~UserAclDbRepoBase() = default;

   /// User
   virtual web::entity::UserDtoList getUsers(const sql::SqlQueryOption& option={}) = 0;
   virtual web::entity::UserDto getUserById(const long long id) = 0;
   virtual bool userExists(const long long id) = 0;
   virtual bool deleteUser(const long long id) = 0;
   virtual bool insertUser(const web::entity::UserDto& user, const std::string& password) = 0;
   virtual bool updateUser(const web::entity::UserDto& user) = 0;
   virtual bool resetUserPassword(long userId, const std::string& newPassword) = 0;

   /// Role
   virtual web::entity::RoleList getRoles(const sql::SqlQueryOption& option={}) = 0;
   virtual web::entity::Role getRoleById(const long long id) = 0;
   virtual bool roleExists(const long long id) = 0;
   virtual bool deleteRole(const long long id) = 0;
   virtual bool insertRole(const web::entity::Role& role) = 0;
   virtual bool updateRole(const web::entity::Role& role) = 0;
   virtual web::entity::UserSimpleList getNonRoleMembers(long id) = 0;
   virtual web::entity::UserSimpleList getRoleMembers(long id) = 0;
   virtual bool roleAddUsers(const web::entity::RoleAddUser& data) = 0;
   virtual bool roleRemoveUser(long roleId, long userId) = 0;

   /// Menu
   virtual web::entity::MenuList getMenus(const sql::SqlQueryOption& option={}) = 0;
   virtual web::entity::MenuViewList getMenuViews(const sql::SqlQueryOption& option={}) = 0;
   virtual web::entity::Menu getMenuById(const long long id) = 0;
   virtual bool menuExists(const long long id) = 0;
   virtual bool deleteMenu(const long long id) = 0;
   virtual bool insertMenu(const web::entity::Menu& menu) = 0;
   virtual bool updateMenu(const web::entity::Menu& menu) = 0;   
   virtual web::entity::MenuTypeViewList getMenuTypes() = 0;
   virtual web::entity::MenuGroupViewList getMenuGroups() = 0;

   /// Acl
   virtual web::entity::AclViewList getAcls(const sql::SqlQueryOption& option={}) = 0;
   virtual web::entity::AclView getAclById(const long long id) = 0;
   virtual bool aclExists(const long long id) = 0;
   virtual bool deleteAcl(const long long id) = 0;
   virtual bool insertAcl(const web::entity::Acl& acl) = 0;
   virtual bool updateAcl(const web::entity::Acl& acl) = 0;   
};

using UserAclDbRepoPtr = std::shared_ptr<UserAclDbRepoBase>;

} // namespace app
} // namespace tbs
