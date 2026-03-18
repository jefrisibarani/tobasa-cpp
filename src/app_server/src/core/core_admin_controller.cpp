#include <fstream>
#include <tobasa/config.h>
#include <tobasa/json.h>
#include <tobasaweb/settings_webapp.h>
#include <tobasaweb/alert.h>
#include <tobasaweb/session.h>
#include <tobasaweb/credential_info.h>
#include <tobasaweb/entity/user.h>
#include <tobasaweb/exception.h>
#include "../api_result.h"
#include "../app_resource.h"
#include "../app_util.h"
#include "core_admin_controller.h"

namespace tbs {
namespace app {

using namespace http;
using namespace web;

using sc = http::StatusCode;

AdminController::AdminController(app::DbServicePtr dbService)
   : web::ControllerBase()
   , _dbService {dbService}
{
   _menuGroup.icon      = "fas fa-toolbox";
   _menuGroup.groupName = "admin";
   _menuGroup.caption   = "Admin tools";
   _menuGroup.menuList.emplace_back("Users",  "admin/users", "fas fa-user");
   _menuGroup.menuList.emplace_back("Groups", "admin/roles", "fas fa-users");
   _menuGroup.menuList.emplace_back("Menu",   "admin/menus", "fas fa-bars");
   _menuGroup.menuList.emplace_back("Acl",    "admin/acl",   "fas fa-check-circle");

   web::Page::menuGroupList.emplace_back(_menuGroup);
}

void AdminController::bindHandler()
{
   auto self(this);
   using namespace std::placeholders;

   //! Handle GET request to /admin/users
   router()->httpGet("/admin/users",
      std::bind(&AdminController::onUsers, self, _1),                  AuthScheme::COOKIE);
   
   //! Handle POST request to /api/admin/users?password={password}
   router()->httpPost("/api/admin/users",
      std::bind(&AdminController::onUsersPost, self, _1),              AuthScheme::BEARER);
   
   //! Handle DEL request to /api/admin/users
   router()->httpDelete("/api/admin/users",
      std::bind(&AdminController::onUsersDel, self, _1),               AuthScheme::BEARER);
   
   //! Handle POST request to /api/admin/users/reset_password
   router()->httpPost("/api/admin/users/reset_password",
      std::bind(&AdminController::onUsersResetPasswordPost, self, _1), AuthScheme::BEARER);

   //! Handle GET request to /admin/roles
   router()->httpGet("/admin/roles",
      std::bind(&AdminController::onRoles, self, _1),                  AuthScheme::COOKIE);
   
   //! Handle POST request to /api/admin/roles
   router()->httpPost("/api/admin/roles",
      std::bind(&AdminController::onRolesPost, self, _1),              AuthScheme::BEARER);

   //! Handle DEL request to /api/admin/roles
   router()->httpDelete("/api/admin/roles",
      std::bind(&AdminController::onRolesDel, self, _1),               AuthScheme::BEARER);
   
   //! Handle GET request to /api/admin/roles/get_non_member?roleid={roleid}
   router()->httpGet("/api/admin/roles/get_non_member",
      std::bind(&AdminController::onRolesGetNonMember, self, _1),      AuthScheme::BEARER);
   
   //! Handle GET request to /api/admin/roles/get_member?roleid={roleid}
   router()->httpGet("/api/admin/roles/get_member",
      std::bind(&AdminController::onRolesGetMember, self, _1),         AuthScheme::BEARER);
   
   //! Handle POST request to /api/admin/roles/addusers
   router()->httpPost("/api/admin/roles/addusers",
      std::bind(&AdminController::onRolesAddUserPost, self, _1),       AuthScheme::BEARER);
   
   //! Handle POST request to /api/admin/roles/remove_user
   router()->httpPost("/api/admin/roles/remove_user",
      std::bind(&AdminController::onRolesRemoveUserPost, self, _1),    AuthScheme::BEARER);


   //! Handle GET request to /admin/menus
   router()->httpGet("/admin/menus",
      std::bind(&AdminController::onMenus, self, _1),                  AuthScheme::COOKIE);
   
   //! Handle POST request to /api/admin/menus
   router()->httpPost("/api/admin/menus",
      std::bind(&AdminController::onMenusPost, self, _1),              AuthScheme::BEARER);
   
   //! Handle DEL request to /api/admin/menus
   router()->httpDelete("/api/admin/menus",
      std::bind(&AdminController::onMenusDel, self, _1),               AuthScheme::BEARER);


   //! Handle GET request to /admin/acl
   router()->httpGet("/admin/acl",
      std::bind(&AdminController::onAcl, self, _1),                    AuthScheme::COOKIE);
   
   //! Handle POST request to /api/admin/acl
   router()->httpPost("/api/admin/acl",
      std::bind(&AdminController::onAclPost, self, _1),                AuthScheme::BEARER);
   
   //! Handle DEL request to /api/admin/acl
   router()->httpDelete("/api/admin/acl",
      std::bind(&AdminController::onAclDel, self, _1),                 AuthScheme::BEARER);
}


//! Handle GET request to /admin/users
http::ResultPtr AdminController::onUsers(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   try
   {
      auto users = _dbService->createUserAclDbRepo()->getUsers();
      web::dom::DatatableOpt datatableopt;

      auto page = std::make_shared<web::Page>(httpCtx);
      page->data("pageMenuAdmin",    _menuGroup);
      page->data("pageTitle",        "Users");
      page->data("pageLang",         "en");
      page->data("pageBodyClass",    "bg-primary");
      page->data("pageId",           "id_admin_users");
      page->data("pageContentTitle", "Users");
      page->data("sessExpNotice",    60);
      page->data("sessExpTime",      authResult.expirationTime);
      page->data("pageBreadcrumb",   "Home / Users");
      page->data("datatableId",      "id_table_users");
      page->data("datatable",         datatableopt);

      page->data()["identity"]["userName"]  = authResult.identity.pUser->userName;
      page->data("users", users);

      return page->show("admin_users.tpl");
   }
   catch(const std::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] onUsers, {}", httpCtx->connId(), ex.what());
      return statusResultHtml(StatusCode::INTERNAL_SERVER_ERROR);
   }
}

//! Handle POST request to /api/admin/users?password={password}
http::ResultPtr AdminController::onUsersPost(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   auto query = httpCtx->request()->query();

   try
   {
      auto body = httpCtx->request()->content();
      auto dto  = Json::parse(body);

      bool result = false;
      entity::UserDto user = dto.get<entity::UserDto>();

      if (user.uniqueCode.empty())
         return web::badParameter("Invalid value for parameter unique code");
      if (user.phone.empty())
         return web::badParameter("Invalid value for parameter phone number");
      if (user.address.empty())
         return web::badParameter("Invalid value for parameter address");
      if (user.nik.empty())
         return web::badParameter("Invalid value for parameter nik");
      if (user.birthDate.empty())
         return web::badParameter("Invalid value for parameter birth date");
      if (user.userName.empty())
         return web::badParameter("User name is required");
      if (user.userName.length() < 4 )
         return web::badParameter("User name minimum length is 4 characters");
      if (user.firstName.empty())
         return web::badParameter("Bad parameter, user first name");
      if (user.lastName.empty())
         return web::badParameter("Bad parameter, user last name");
      if (user.email.empty())
         return web::badParameter("Bad parameter, email Address");

      if (user.birthDate.empty())
         user.birthDate = "1900-01-01";
      else
      {
         DateTime birthdate;
         if (!birthdate.parse(user.birthDate, "%Y-%m-%d"))
            return web::badParameter("Invalid birth date format");
      }

      if (user.expired.empty())
      {
         auto currentTime = DateTime();
         currentTime.timePoint() += tbsdate::years{2};
         DateTime expiredDate(currentTime.timePoint());
         user.expired = expiredDate.isoDateTimeString();
      }
      else
      {
         DateTime expired;
         if (!expired.parse(user.expired, "%Y-%m-%d %H:%M:%S"))
            return web::badParameter("Invalid expired date time format");
      }

      auto authDbRepo    = _dbService->createAuthDbRepo();
      auto userAclDbRepo = _dbService->createUserAclDbRepo();

      // remove non printable char
      util::removeTrailingWhiteSpace(user.nik);

      auto targetUser = userAclDbRepo->getUserById(user.id);
      if (targetUser.userName == "admin")
      {
         if (user.userName != "admin")
            return web::failed("Admin username changes are not permitted");
         if (!user.allowLogin)
            return web::failed("You are not permitted to block the admin user");
         if (!user.enabled)
            return web::failed("You are not permitted to disable the admin user");
      }

      if ( (user.id > 0 ) && targetUser.id == user.id )
      {
         user.uuid = targetUser.uuid;
         if ( userAclDbRepo->updateUser(user) )
            return web::okResult();
         else
            return web::failed("User update failed");
      }
      else
      {
         if ( ! query->hasField("password") ) 
            return web::badParameter("Invalid parameter password");

         auto password = query->value("password");
         if (password.empty())
            return web::badParameter("Password is required");
         if (password.length() < 5)
            return web::badParameter("Password minimum length is 5 characters");
         if ( authDbRepo->exists(user.userName))
            return web::badParameter("User name " + user.userName + " is already taken");
         if ( authDbRepo->uniqueCodeExists(user.uniqueCode))
            return web::badParameter("Unique code " + user.uniqueCode + " is already taken");
         if ( authDbRepo->nikExists(user.nik))
            return web::badParameter("NIK " + user.nik + " is already taken");

         userAclDbRepo->beginTransaction();
         
         if (! userAclDbRepo->insertUser(user, password))
         {
            userAclDbRepo->rollbackTransaction();
            return web::failed("User insertion failed");
         }

         auto pNewUSer = authDbRepo->getUserByName(user.userName);
         if (pNewUSer = nullptr)
         {
            userAclDbRepo->rollbackTransaction();
            return web::failed("User insertion failed");
         }

         if (! authDbRepo->insertIntoBaseUserRole(pNewUSer))
         {
            userAclDbRepo->rollbackTransaction();
            return web::failed("User insertion failed");
         }

         //long selectedSiteId = user.selectedSiteId;
         long selectedSiteId = 1;
         if (! authDbRepo->insertIntoBaseUserSite(pNewUSer, selectedSiteId))
         {
            userAclDbRepo->rollbackTransaction();
            return web::failed("User insertion failed");
         }

         userAclDbRepo->commitTransaction();
         return web::okResult();
      }
   }
   catch (const Json::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] {}", httpCtx->connId(), ex.what());
      return web::badRequest("JSON data error: " + cleanJsonException(ex));
   }
   catch(const std::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] onUsersPost {}", httpCtx->connId(), ex.what());
      return web::appError();
   }
}

//! Handle DEL request to /api/admin/users
http::ResultPtr AdminController::onUsersDel(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   try
   {
      auto body = httpCtx->request()->content();
      auto dto  = Json::parse(body);
      auto id   = dto["id"].get<long>();

      auto userAclDbRepo = _dbService->createUserAclDbRepo();

      auto user = userAclDbRepo->getUserById(id);
      if (user.userName == "admin")
         return web::failed("Admin user deletion is not allowed");

      bool res = userAclDbRepo->deleteUser(id);
      if (res)
         return web::okResult();
      else
         return web::failed("Failed to delete the user");
   }
   catch(const std::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] onUsersDel, {}", httpCtx->connId(), ex.what());
      return web::appError();
   }
}

//! Handle POST request to /api/admin/users/reset_password
http::ResultPtr AdminController::onUsersResetPasswordPost(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );

   // TODO_JEFRI: validate also session/http request user/operator
   //authResult.identity.pUser->id;

   auto body = httpCtx->request()->content();
   auto dto  = Json::parse(body);
   auto targetUserId = dto["userId"].get<long>();
   auto newPassword  = dto["password"].get<std::string>();

   auto userAclDbRepo = _dbService->createUserAclDbRepo();
   bool res = userAclDbRepo->resetUserPassword(targetUserId, newPassword);
   if (res)
      return web::okResult();
   else
      return web::failed("Reset user password failed");
}


//! Handle GET request to /admin/roles
http::ResultPtr AdminController::onRoles(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   try
   {
      auto userAclDbRepo = _dbService->createUserAclDbRepo();
      auto roles = userAclDbRepo->getRoles();
      web::dom::DatatableOpt datatableopt;

      auto page = std::make_shared<web::Page>(httpCtx);
      page->data("pageMenuAdmin",    _menuGroup);
      page->data("pageTitle",        "Groups");
      page->data("pageLang",         "en");
      page->data("pageBodyClass",    "bg-primary");
      page->data("pageId",           "id_admin_groups");
      page->data("pageContentTitle", "Groups");
      page->data("sessExpNotice",    60);
      page->data("sessExpTime",      authResult.expirationTime);
      page->data("pageBreadcrumb",   "Home / Groups");
      page->data("datatableId",      "id_table_groups");
      page->data("datatable",         datatableopt);
      page->data()["identity"]["userName"]  = authResult.identity.pUser->userName;
      page->data("roles", roles);

      return page->show("admin_roles.tpl");
   }
   catch(const std::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] onRoles, {}", httpCtx->connId(), ex.what());
      return statusResultHtml(StatusCode::INTERNAL_SERVER_ERROR);
   }
}

//! Handle POST request to /api/admin/roles
http::ResultPtr AdminController::onRolesPost(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   try
   {
      auto body = httpCtx->request()->content();
      auto dto  = Json::parse(body);

      bool result = false;
      entity::Role role = dto.get<entity::Role>();
      
      auto userAclDbRepo = _dbService->createUserAclDbRepo();

      auto targetRole = userAclDbRepo->getRoleById(role.id);
      if (targetRole.sysRole)
         return web::failed("You are not permitted to update system role");

      if ( (role.id > 0) && targetRole.id == role.id )
         result = userAclDbRepo->updateRole(role);
      else
         result = userAclDbRepo->insertRole(role);

      if (result)
         return web::okResult();
      else
         return web::failed("Role update/insertion failed");
   }
   catch(const std::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] onRolesPost, {}", httpCtx->connId(), ex.what());
      return web::appError();
   }
}

//! Handle DEL request to /api/admin/roles
http::ResultPtr AdminController::onRolesDel(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   try
   {
      auto body = httpCtx->request()->content();
      auto dto  = Json::parse(body);
      auto id   = dto["id"].get<long>();

      auto userAclDbRepo = _dbService->createUserAclDbRepo();

      auto role = userAclDbRepo->getRoleById(id);
      if (role.sysRole)
         return web::failed("System role deletion is not allowed");

      bool res = userAclDbRepo->deleteRole(id);
      if (res)
         return web::okResult();
      else
         return web::failed("Failed to delete the role");
   }
   catch(const std::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] onRolesDel, {}", httpCtx->connId(), ex.what());
      return web::appError();
   }
}

//! Handle GET request to /api/admin/roles/get_non_member/?roleid={roleid}
http::ResultPtr AdminController::onRolesGetNonMember(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   auto query = httpCtx->request()->query();
   try
   {
      if ( query->hasField("roleid") )
      {
         auto roleid = query->value("roleid");
         if (!roleid.empty())
         {
            if (util::isNumber(roleid))
            {
               auto result = _dbService->createUserAclDbRepo()->getNonRoleMembers(std::stol(roleid));
               return web::object(result);
            }
         }
      }

      return web::badParameter("parameter role id is required");
   }
   catch(const std::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] onRolesGetNonMember, {}", httpCtx->connId(), ex.what());
      return web::appError();
   }
}

//! Handle GET request to /api/admin/roles/get_member/?roleid={roleid}
http::ResultPtr AdminController::onRolesGetMember(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   auto query = httpCtx->request()->query();
   try
   {
      if ( query->hasField("roleid") )
      {
         auto roleid = query->value("roleid");
         if (!roleid.empty())
         {
            if (util::isNumber(roleid))
            {
               auto result = _dbService->createUserAclDbRepo()->getRoleMembers(std::stol(roleid));
               return web::object(result);
            }
         }
      }

      return web::badParameter("parameter role id is required");
   }
   catch(const std::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] onRolesGetMember, {}", httpCtx->connId(), ex.what());
      return web::appError();
   }
}


//! Handle POST request to /api/admin/roles/addusers
http::ResultPtr AdminController::onRolesAddUserPost(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   try
   {
      auto body    = httpCtx->request()->content();
      auto dto     = Json::parse(body);
      auto payload = dto.get<RoleAddUser>();

      if (payload.userIds.size() == 0)
         return web::failed("No users in the data");

      auto userAclDbRepo = _dbService->createUserAclDbRepo();   

      if ( userAclDbRepo->roleExists(payload.roleId) )
      {
         bool result = userAclDbRepo->roleAddUsers(payload);
         if (result)
            return web::okResult();
         else
            return web::failed("Could not add users into role");
      }

      return web::failed("Role does not exist");
   }
   catch(const std::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] onRolesAddUserPost, {}", httpCtx->connId(), ex.what());
      return web::appError();
   }
}

//! Handle POST request to /api/admin/roles/remove_user
http::ResultPtr AdminController::onRolesRemoveUserPost(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   try
   {
      auto body = httpCtx->request()->content();
      auto dto  = Json::parse(body);
      auto roleId = dto["roleId"].get<long>();
      auto userId = dto["userId"].get<long>();

      auto userAclDbRepo = _dbService->createUserAclDbRepo();

      if ( userAclDbRepo->roleExists(roleId) )
      {
         bool result = userAclDbRepo->roleRemoveUser(roleId, userId);
         if (result)
            return web::okResult();
         else
            return web::failed("Could not remove user");
      }

      return web::failed("Role does not exist");
   }
   catch(const std::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] onRolesRemoveUserPost, {}", httpCtx->connId(), ex.what());
      return web::appError();
   }
}


//! Handle GET request to /admin/menus
http::ResultPtr AdminController::onMenus(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   try
   {
      auto userAclDbRepo = _dbService->createUserAclDbRepo();

      auto menus = userAclDbRepo->getMenus();
      auto menuTypes = userAclDbRepo->getMenuTypes();
      auto menuGroups = userAclDbRepo->getMenuGroups();
      web::dom::DatatableOpt datatableopt;

      auto page = std::make_shared<web::Page>(httpCtx);
      page->data("pageMenuAdmin",    _menuGroup);
      page->data("pageTitle",        "Menus");
      page->data("pageLang",         "en");
      page->data("pageBodyClass",    "bg-primary");
      page->data("pageId",           "id_admin_menus");
      page->data("pageContentTitle", "Menus");
      page->data("sessExpNotice",    60);
      page->data("sessExpTime",      authResult.expirationTime);
      page->data("pageBreadcrumb",   "Home / Menus");
      page->data("datatableId",      "id_table_menus");
      page->data("datatable",        datatableopt);
      page->data("menus",            menus);
      page->data("menuTypes",        menuTypes);
      page->data("menuGroups",       menuGroups);
      page->data()["identity"]["userName"] = authResult.identity.pUser->userName;

      return page->show("admin_menus.tpl");
   }
   catch(const std::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] onMenus, {}", httpCtx->connId(), ex.what());
      return statusResultHtml(StatusCode::INTERNAL_SERVER_ERROR);
   }
}

//! Handle POST request to /api/admin/menus
http::ResultPtr AdminController::onMenusPost(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   try
   {
      auto body = httpCtx->request()->content();
      auto dto  = Json::parse(body);

      auto userAclDbRepo = _dbService->createUserAclDbRepo();
      bool result = false;
      entity::Menu menu = dto.get<entity::Menu>();
      if ( userAclDbRepo->menuExists (menu.id) )
         result = userAclDbRepo->updateMenu(menu);
      else
         result = userAclDbRepo->insertMenu(menu);

      if (result)
         return web::okResult();
      else
         return web::failed("Failed updating/inserting menu");
   }
   catch(const std::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] onMenusPost, {}", httpCtx->connId(), ex.what());
      return web::appError();
   }
}

//! Handle DEL request to /api/admin/menus
http::ResultPtr AdminController::onMenusDel(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   try
   {
      auto body = httpCtx->request()->content();
      auto dto  = Json::parse(body);
      auto id   = dto["id"].get<long>();

      bool res = _dbService->createUserAclDbRepo()->deleteMenu(id);
      if (res)
         return web::okResult();
      else
         return web::failed("Failed delete menu");
   }
   catch(const std::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] onMenusDel, {}", httpCtx->connId(), ex.what());
      return web::appError();
   }
}


//! Handle GET request to /admin/acl
http::ResultPtr AdminController::onAcl(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   try
   {
      // ---------------------------------------------
      auto userAclDbRepo = _dbService->createUserAclDbRepo();
      auto roles = userAclDbRepo->getRoles();
      auto users = userAclDbRepo->getUsers();
      Json usersAndRoles = Json::array();
      for (auto r: roles)
      {
         Json obj;
         obj["id"]   = "G_" + std::to_string(r.id);
         obj["code"] = "Role | " + r.name;
         obj["name"] = "";
         usersAndRoles.push_back(obj);
      }

      for (auto u: users)
      {
         Json obj;
         obj["id"]   = "U_" + std::to_string(u.id);
         obj["code"] = "User | " + u.userName;
         obj["name"] = "";
         usersAndRoles.push_back(obj);
      }
      // ---------------------------------------------
      auto menus = userAclDbRepo->getMenuViews();
      Json menuViews = Json::array();
      for (auto m: menus)
      {
         std::string description;
         description = tbsfmt::format("{} | {} | {} | {}", m.name,m.menuType,m.menuGroup,m.pageurl);
         Json obj;
         obj["id"] = m.id;
         obj["code"] = description;
         obj["name"] = "";
         menuViews.push_back(obj);
      }
      // ---------------------------------------------

      auto acls = userAclDbRepo->getAcls();
      web::dom::DatatableOpt datatableopt;

      auto page = std::make_shared<web::Page>(httpCtx);
      page->data("pageMenuAdmin",    _menuGroup);
      page->data("pageTitle",        "Acl");
      page->data("pageLang",         "en");
      page->data("pageBodyClass",    "bg-primary");
      page->data("pageId",           "id_admin_acl");
      page->data("pageContentTitle", "ACL");
      page->data("sessExpNotice",    60);
      page->data("sessExpTime",      authResult.expirationTime);
      page->data("pageBreadcrumb",   "Home / ACL");
      page->data("datatableId",      "id_table_acl");
      page->data("datatable",         datatableopt);

      page->data("dataAcls", acls);
      page->data("dataMenus", menus);
      page->data("dataMenuViews", menuViews);
      page->data("dataRolesUAndsers", usersAndRoles);

      page->data()["identity"]["userName"]  = authResult.identity.pUser->userName;

      return page->show("admin_acls.tpl");
   }
   catch(const std::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] onAcl, {}", httpCtx->connId(), ex.what());
      return statusResultHtml(StatusCode::INTERNAL_SERVER_ERROR);
   }
}

//! Handle POST request to /api/admin/acl
http::ResultPtr AdminController::onAclPost(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   try
   {
      auto body = httpCtx->request()->content();
      auto dto  = Json::parse(body);

      bool result = false;
      entity::Acl acl = dto.get<entity::Acl>();

      if (! (acl.ugType == "U" ||  acl.ugType == "G") )
         return web::badParameter("Invalid value for user/group code");

      auto userAclDbRepo = _dbService->createUserAclDbRepo();   

      if ( userAclDbRepo->aclExists (acl.id) )
         result = userAclDbRepo->updateAcl(acl);
      else
         result = userAclDbRepo->insertAcl(acl);

      if (result)
         return web::okResult();
      else
         return web::failed("Failed updating/inserting acl");
   }
   catch(const std::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] onAclPost, {}", httpCtx->connId(), ex.what());
      return web::appError();
   }
}

//! Handle DEL request to /api/admin/acl
http::ResultPtr AdminController::onAclDel(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   try
   {
      auto body = httpCtx->request()->content();
      auto dto  = Json::parse(body);
      auto id   = dto["id"].get<long>();

      bool res = _dbService->createUserAclDbRepo()->deleteAcl(id);
      if (res)
         return web::okResult();
      else
         return web::failed("Failed delete acl");
   }
   catch(const std::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] onAclDel, {}", httpCtx->connId(), ex.what());
      return web::appError();
   }
}


} // namespace app
} // namespace tbs