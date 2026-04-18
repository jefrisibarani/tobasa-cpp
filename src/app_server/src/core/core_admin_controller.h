#pragma once

#include <tobasasql/settings.h>
#include <tobasaweb/controller_base.h>
#include <tobasaweb/router.h>
#include "../database_service_factory_app.h"
#include "../page.h"

namespace tbs {
namespace app {

class AdminController
   : public web::ControllerBase
{
public:
   AdminController(const AdminController &) = delete;
   AdminController(AdminController &&) = delete;

   explicit AdminController(app::DbServicePtr dbService);
   virtual ~AdminController() {}

   //! Handle GET request to /admin/users
   http::ResultPtr onUsers(const web::RouteArgument& arg);
   //! Handle POST request to /api/admin/users?password={password}
   http::ResultPtr onUsersPost(const web::RouteArgument& arg);
   //! Handle DEL request to /api/admin/users
   http::ResultPtr onUsersDel(const web::RouteArgument& arg);
   //! Handle POST request to /api/admin/users/reset_password
   http::ResultPtr onUsersResetPasswordPost(const web::RouteArgument& arg);

   //! Handle GET request to /admin/roles
   http::ResultPtr onRoles(const web::RouteArgument& arg);
   //! Handle POST request to /api/admin/roles
   http::ResultPtr onRolesPost(const web::RouteArgument& arg);
   //! Handle DEL request to /api/admin/roles
   http::ResultPtr onRolesDel(const web::RouteArgument& arg);
   //! Handle GET request to /api/admin/roles/get_non_member/?roleid={roleid}
   http::ResultPtr onRolesGetNonMember(const web::RouteArgument& arg);
   //! Handle GET request to /api/admin/roles/get_member/?roleid={roleid}
   http::ResultPtr onRolesGetMember(const web::RouteArgument& arg);
   //! Handle POST request to /api/admin/roles/addusers
   http::ResultPtr onRolesAddUserPost(const web::RouteArgument& arg);
   //! Handle POST request to /api/admin/roles/remove_user
   http::ResultPtr onRolesRemoveUserPost(const web::RouteArgument& arg);


   //! Handle GET request to /admin/menus
   http::ResultPtr onMenus(const web::RouteArgument& arg);
   //! Handle POST request to /api/admin/menus
   http::ResultPtr onMenusPost(const web::RouteArgument& arg);
   //! Handle DEL request to /api/admin/menus
   http::ResultPtr onMenusDel(const web::RouteArgument& arg);

   //! Handle GET request to /admin/acl
   http::ResultPtr onAcl(const web::RouteArgument& arg);
   //! Handle POST request to /api/admin/acl
   http::ResultPtr onAclPost(const web::RouteArgument& arg);
   //! Handle DEL request to /api/admin/acl
   http::ResultPtr onAclDel(const web::RouteArgument& arg);

protected:
   void bindHandler();
   web::dom::MenuGroup _menuGroup;
   app::DbServicePtr _dbService {nullptr};
};


} // namespace app
} // namespace tbs