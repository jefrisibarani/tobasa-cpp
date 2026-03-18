#pragma once

#include <tobasa/logger.h>
#include <tobasa/crypt.h>
#include <tobasa/util.h>
#include <tobasasql/sql_connection.h>
#include <tobasasql/sql_query.h>
#include "tobasaweb/exception.h"
#include "tobasaweb/util.h"
#include "db_repo_user_acl_base.h"

namespace tbs {
namespace app {

/**
 * Database Repository.
 * Database repository for tobasa web app
 */
template < typename SqlDriverType >
class UserAclDbRepo
   : public UserAclDbRepoBase
{
public:
   using SqlResult     = sql::SqlResult<SqlDriverType>;
   using SqlConnection = sql::SqlConnection<SqlDriverType>;
   using SqlQuery      = sql::SqlQuery<SqlDriverType>;
   using Helper        = typename SqlDriverType::HelperImpl;

private:
   SqlConnection& _sqlConn;

public:
   UserAclDbRepo( const UserAclDbRepo & ) = delete;
   UserAclDbRepo( UserAclDbRepo && ) = delete;
   UserAclDbRepo() = default;
   virtual ~UserAclDbRepo() = default;

   UserAclDbRepo(SqlConnection& conn) 
      : _sqlConn{ conn } {}
// -------------------------------------------------------

   web::entity::UserDtoList getUsers(const sql::SqlQueryOption& option={}) 
   {
      std::string sql = "SELECT * FROM base_users ";
      if ( option.limitOffsetValid() ) {
         sql += Helper::limitAndOffset(option.limit, option.offset, _sqlConn.dbmsName());
      }

      SqlQuery query(_sqlConn, sql);
      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         web::entity::UserDtoList items;
         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);
            items.emplace_back( getUser(sqlResult) );
         }

         return items;
      }

      return web::entity::UserDtoList();
   }
   
   web::entity::UserDto getUserById(const long long id)
   {
      std::string sql = "SELECT * FROM base_users where id=:id ";
      SqlQuery query(_sqlConn, sql);
      query.addParam("id", sql::DataType::bigint, id);
      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() == 1) {
         return getUser(sqlResult);
      }

      return web::entity::UserDto();
   }

   web::entity::UserDto getUser(std::shared_ptr<SqlResult> sqlResult)
   {
      web::entity::UserDto user;
      user.id           = sqlResult->getLongValue("id");
      user.uuid         = sqlResult->getStringValue("uuid");
      user.userName     = sqlResult->getStringValue("user_name");
      user.firstName    = sqlResult->getStringValue("first_name");
      user.lastName     = sqlResult->getStringValue("last_name");
      user.email        = sqlResult->getStringValue("email");
      user.image        = sqlResult->getStringValue("image");
      user.enabled      = sqlResult->getBoolValue("enabled");
      user.passwordSalt = sqlResult->getStringValue("password_salt");
      user.passwordHash = sqlResult->getStringValue("password_hash");
      user.allowLogin   = sqlResult->getBoolValue("allow_login");
      user.created      = sqlResult->getStringValue("created");
      user.updated      = sqlResult->getStringValue("updated");
      user.expired      = sqlResult->getStringValue("expired");
      user.lastLogin    = sqlResult->getStringValue("last_login");
      user.uniqueCode   = sqlResult->getStringValue("unique_code");
      user.birthDate    = sqlResult->getStringValue("birth_date");
      user.phone        = sqlResult->getStringValue("phone");
      user.gender       = sqlResult->getStringValue("gender");
      user.address      = sqlResult->getStringValue("address");
      user.nik          = sqlResult->getStringValue("nik");

      return std::move(user);
   }

   bool userExists(const long long id)
   {
      std::string sql = "SELECT COUNT(*) FROM base_users where id=:id";
      SqlQuery query(_sqlConn, sql);
      query.addParam("id", sql::DataType::bigint, id);
      auto result = query.executeScalar();
      return (result == "1");
   }

   bool deleteUser(const long long id)
   {
      try
      {
         std::string sql = "DELETE FROM base_users WHERE id=:id";
         SqlQuery query(_sqlConn, sql);
         query.addParam("id", sql::DataType::bigint, id);
         bool success = query.executeVoid();
         return success;
      }
      catch(std::exception& ex)
      {
         Logger::logE("[webapp] UserAclDbRepo delete user, {}", ex.what());
      }

      return false;
   }

   bool resetUserPassword(long userId, const std::string& newPassword)
   {
      if (newPassword.empty())
         throw ValidationException("Password cannot be empty");

      if (newPassword.length() < 5)
         throw ValidationException("Password must be at least 5 characters long");

      if ( !userExists(userId))
         throw ValidationException("User does not exist");

      try
      {
         // create hex encoded hash
         std::string passwordHash, passwordSalt;
         util::createSecureHash(newPassword, passwordHash, passwordSalt);

         std::string sql = "UPDATE base_users SET password_salt=:salt, password_hash=:hash WHERE id=:uid";
         SqlQuery query(_sqlConn, sql);
         query.addParam("salt", sql::DataType::varbinary, passwordSalt, static_cast<long>(passwordSalt.length()/2));
         query.addParam("hash", sql::DataType::varbinary, passwordHash, static_cast<long>(passwordHash.length()/2));
         query.addParam("uid",  sql::DataType::integer,   userId);

         bool result = query.executeVoid();
         return result;
      }
      catch(std::exception& ex)
      {
         Logger::logE("[webapp] UserAclDbRepo resetUserPassword, {}", ex.what());
      }

      return false;
   }

   bool insertUser(const web::entity::UserDto& user, const std::string& password)
   {
      try
      {
         // create hex encoded hash
         std::string passwordHash, passwordSalt;
         util::createSecureHash(password, passwordHash, passwordSalt);

         std::string sql =
            R"-( INSERT INTO base_users (
                  uuid,          user_name,   first_name, last_name,
                  email,         image,       enabled,    password_salt,
                  password_hash, allow_login, expired,    unique_code,
                  birth_date,    phone,       gender,     address, nik )
               VALUES (
                  :uuid, :usrname, :fname,   :lname, :email, :image, :enabled, :salt,
                  :hash, :allow,   :expired, :ucode, :dob,   :phone, :gender,  :address, :nik )  )-";

         SqlQuery query(_sqlConn, sql);
         query.addParam("uuid",    sql::DataType::varchar,   util::generateUniqueId() );
         query.addParam("usrname", sql::DataType::varchar,   user.userName);
         query.addParam("fname",   sql::DataType::varchar,   user.firstName);
         query.addParam("lname",   sql::DataType::varchar,   user.lastName);
         query.addParam("email",   sql::DataType::varchar,   user.email);
         query.addParam("image",   sql::DataType::varchar,   user.image);
         query.addParam("enabled", sql::DataType::boolean,   user.enabled);
         query.addParam("salt",    sql::DataType::varbinary, passwordSalt, static_cast<long>(passwordSalt.length()/2));
         query.addParam("hash",    sql::DataType::varbinary, passwordHash, static_cast<long>(passwordHash.length()/2));
         query.addParam("allow",   sql::DataType::boolean,   user.allowLogin);
         query.addParam("expired", sql::DataType::timestamp, user.expired);
         query.addParam("ucode",   sql::DataType::varchar,   user.uniqueCode);
         query.addParam("dob",     sql::DataType::date,      user.birthDate);
         query.addParam("phone",   sql::DataType::varchar,   user.phone);
         query.addParam("gender",  sql::DataType::varchar,   user.gender);
         query.addParam("address", sql::DataType::varchar,   user.address);
         query.addParam("nik",     sql::DataType::varchar,   user.nik);

         bool success = query.executeVoid();
         return success;
      }
      catch(std::exception& ex)
      {
         Logger::logE("[webapp] UserAclDbRepo create User, {}", ex.what());
      }

      return false;
   }

   bool updateUser(const web::entity::UserDto& user)
   {
      try
      {
         std::string sql =
            R"-( UPDATE base_users SET
                     uuid = :uuid, user_name = :uname, first_name = :fname, last_name = :lname,
                     email = :email, image = :image, enabled = :enabled, allow_login = :allow,
                     updated = :updated, expired = :expired, unique_code = :ucode, birth_date = :dob,
                     phone = :phone, gender = :gender, address = :address, nik = :nik
                  WHERE id=:id )-";

         SqlQuery query(_sqlConn, sql);
         query.addParam("uuid",    sql::DataType::varchar,   user.uuid);
         query.addParam("uname",   sql::DataType::varchar,   user.userName);
         query.addParam("fname",   sql::DataType::varchar,   user.firstName);
         query.addParam("lname",   sql::DataType::varchar,   user.lastName);
         query.addParam("email",   sql::DataType::varchar,   user.email);
         query.addParam("image",   sql::DataType::varchar,   user.image);
         query.addParam("enabled", sql::DataType::boolean,   user.enabled);
         query.addParam("allow",   sql::DataType::boolean,   user.allowLogin);
         query.addParam("updated", sql::DataType::timestamp, DateTime().isoDateTimeString());
         query.addParam("expired", sql::DataType::timestamp, user.expired);
         //query.addParam("llogin",sql::DataType::timestamp, user.lastLogin);
         query.addParam("ucode",   sql::DataType::varchar,   user.uniqueCode);
         query.addParam("dob",     sql::DataType::date,      user.birthDate);
         query.addParam("phone",   sql::DataType::varchar,   user.phone);
         query.addParam("gender",  sql::DataType::varchar,   user.gender);
         query.addParam("address", sql::DataType::varchar,   user.address);
         query.addParam("nik",     sql::DataType::varchar,   user.nik);
         query.addParam("id",      sql::DataType::integer,   user.id);

         bool success = query.executeVoid();
         return success;
      }
      catch( const std::exception& ex)
      {
         Logger::logE("[webapp] UserAclDbRepo update User, {}", ex.what());
      }

      return false;
   }


// -------------------------------------------------------

   web::entity::RoleList getRoles(const sql::SqlQueryOption& option={}) 
   {
      std::string sql = "SELECT * FROM base_roles ";
      if ( option.limitOffsetValid() ) {
         sql += Helper::limitAndOffset(option.limit, option.offset, _sqlConn.dbmsName());
      }

      SqlQuery query(_sqlConn, sql);
      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         web::entity::RoleList items;
         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);
            items.emplace_back( getRole(sqlResult) );
         }

         return items;
      }

      return web::entity::RoleList();
   }
   
   web::entity::Role getRoleById(const long long id)
   {
      std::string sql = "SELECT * FROM base_roles where id=:id";
      SqlQuery query(_sqlConn, sql);
      query.addParam("id", sql::DataType::bigint, id);

      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() == 1)
      {
         return getRole(sqlResult);
      }

      return web::entity::Role();
   }

   web::entity::Role getRole(std::shared_ptr<SqlResult> sqlResult)
   {
      web::entity::Role role;

      role.id = sqlResult->getLongValue("id");
      role.name = sqlResult->getStringValue("name");
      role.alias = sqlResult->getStringValue("alias");
      role.enabled = sqlResult->getBoolValue("enabled");
      role.sysRole = sqlResult->getBoolValue("sysrole");
      //role.created = sqlResult->getStringValue("created");
      //role.modified = sqlResult->getStringValue("modified");

      return std::move(role);
   }   

   bool roleExists(const long long id)
   {
      std::string sql = "SELECT COUNT(*) FROM base_roles where id=:id";
      SqlQuery query(_sqlConn, sql);
      query.addParam("id", sql::DataType::bigint, id);
      auto result = query.executeScalar();
      return (result == "1");
   }

   bool deleteRole(const long long id)
   {
      try
      {
         std::string sql = "DELETE FROM base_roles WHERE id=:id";
         SqlQuery query(_sqlConn, sql);
         query.addParam("id", sql::DataType::bigint, id);
         bool success = query.executeVoid();
         return success;
      }
      catch(std::exception& ex)
      {
         Logger::logE("[webapp] UserAclDbRepo delete Role, {}", ex.what());
      }

      return false;
   }

   bool insertRole(const web::entity::Role& role)
   {
      try
      {
         std::string sql = 
         R"-( INSERT INTO base_roles (name, alias, enabled, sysrole)
                  VALUES ( :name, :alias, :enabled, :sysrole )  )-" ;
   
         SqlQuery query(_sqlConn, sql);
         query.addParam("name",    sql::DataType::varchar, role.name);
         query.addParam("alias",   sql::DataType::varchar, role.alias);
         query.addParam("enabled", sql::DataType::boolean, role.enabled);
         query.addParam("sysrole", sql::DataType::boolean, role.sysRole);

         bool success = query.executeVoid();
         return success;
      }
      catch( const std::exception& ex)
      {
         Logger::logE("[webapp] UserAclDbRepo insert Role, {}", ex.what());
      }

      return false;
   }

   bool updateRole(const web::entity::Role& role)
   {
      try
      {
         std::string sql =
            R"-( UPDATE base_roles SET
                    name    = :name,    alias   = :alias,
                    enabled = :enabled, sysrole = :sysrole
                  WHERE id = :id )-";

         SqlQuery query(_sqlConn, sql);
         query.addParam("name",    sql::DataType::varchar, role.name);
         query.addParam("alias",   sql::DataType::varchar, role.alias);
         query.addParam("enabled", sql::DataType::boolean, role.enabled);
         query.addParam("sysrole", sql::DataType::boolean, role.sysRole);
         query.addParam("id",      sql::DataType::integer, role.id);

         bool success = query.executeVoid();
         return success;
      }
      catch( const std::exception& ex)
      {
         Logger::logE("[webapp] UserAclDbRepo update Role, {}", ex.what());
      }

      return false;
   }

   web::entity::UserSimple getUserSimple(std::shared_ptr<SqlResult> sqlResult)
   {
      web::entity::UserSimple user;
      user.id           = sqlResult->getLongValue("id");
      user.userName     = sqlResult->getStringValue("user_name");
      user.firstName    = sqlResult->getStringValue("first_name");
      user.lastName     = sqlResult->getStringValue("last_name");
      user.email        = sqlResult->getStringValue("email");
      user.enabled      = sqlResult->getBoolValue("enabled");

      return std::move(user);
   }  

   web::entity::UserSimpleList getNonRoleMembers(long id)
   {
      std::string sql = R"-(
                SELECT id, user_name, first_name, last_name, email, enabled
                FROM base_users WHERE id NOT IN 
                 (SELECT user_id FROM base_user_role WHERE role_id=:id) )-";

      SqlQuery query(_sqlConn, sql);
      query.addParam("id", sql::DataType::integer, id);

      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         web::entity::UserSimpleList items;

         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);
            items.emplace_back( getUserSimple(sqlResult) );
         }

         return items;
      }

      return web::entity::UserSimpleList();
   }   

   web::entity::UserSimpleList getRoleMembers(long id)
   {
      std::string sql = R"-(
                SELECT id, user_name, first_name, last_name, email, enabled
                FROM base_users WHERE id IN 
                (SELECT user_id FROM base_user_role WHERE role_id=:id)  )-";

      SqlQuery query(_sqlConn, sql);
      query.addParam("id", sql::DataType::integer, id);

      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         web::entity::UserSimpleList items;

         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);
            items.emplace_back( getUserSimple(sqlResult) );
         }

         return items;
      }

      return web::entity::UserSimpleList();
   }   

   bool roleAddUsers(const web::entity::RoleAddUser& data)
   {
      if (data.userIds.size() == 0)
         return false;

      try
      {
         std::string sql("INSERT INTO base_user_role (user_id, role_id) VALUES ");
         std::string values;
         auto roleId = data.roleId;
         for (auto uid: data.userIds)
         {
            if (!values.empty())
               values += ", ";

            values += tbsfmt::format("({},{})", uid, data.roleId);
         }

         sql += values;
         SqlQuery query(_sqlConn, sql);
         bool success = query.executeVoid();
         return success;
      }
      catch( const std::exception& ex)
      {
         Logger::logE("[webapp] UserAclDbRepo roleAddUser, {}", ex.what());
      }

      return false;
   }

   bool roleRemoveUser(long roleId, long userId)
   {
      if (! (roleId>0 && userId>0) )
         return false;

      try
      {
         std::string sql = "DELETE FROM base_user_role WHERE role_id=:rid AND user_id=:uid";
         SqlQuery query(_sqlConn, sql);
         query.addParam("rid", sql::DataType::integer, roleId);
         query.addParam("uid", sql::DataType::integer, userId);

         bool success = query.executeVoid();
         return success;
      }
      catch( const std::exception& ex)
      {
         Logger::logE("[webapp] UserAclDbRepo roleRemoveUser, {}", ex.what());
      }

      return false;
   }

   
// -------------------------------------------------------   
   web::entity::Menu getMenu(std::shared_ptr<SqlResult> sqlResult)
   {
      web::entity::Menu menu;

      menu.id      = sqlResult->getLongValue("id");
      menu.name    = sqlResult->getStringValue("name");
      menu.groupId = sqlResult->getLongValue("group_id");
      menu.typeId  = sqlResult->getLongValue("type_id");
      menu.pageurl = sqlResult->getStringValue("pageurl");
      menu.sidebar = sqlResult->getBoolValue("sidebar");
      menu.methods = sqlResult->getStringValue("methods");

      return std::move(menu);
   }  

   web::entity::MenuList getMenus(const sql::SqlQueryOption& option={})
   {
      std::string sql = "SELECT * FROM base_menu ";

      if ( option.limitOffsetValid() ) {
         sql += Helper::limitAndOffset(option.limit, option.offset, _sqlConn.dbmsName());
      }

      SqlQuery query(_sqlConn, sql);

      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         web::entity::MenuList items;

         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);
            items.emplace_back( getMenu(sqlResult) );
         }

         return items;
      }

      return web::entity::MenuList();
   }

   web::entity::MenuView getMenuView(std::shared_ptr<SqlResult> sqlResult)
   {
      web::entity::MenuView menu;

      menu.id        = sqlResult->getLongValue("id");
      menu.name      = sqlResult->getStringValue("name");
      menu.menuType  = sqlResult->getStringValue("menu_type");
      menu.menuGroup = sqlResult->getStringValue("menu_group");
      menu.pageurl   = sqlResult->getStringValue("pageurl");
      menu.sidebar   = sqlResult->getBoolValue("sidebar");
      menu.methods   = sqlResult->getStringValue("methods");

      return std::move(menu);
   }   

   web::entity::MenuViewList getMenuViews(const sql::SqlQueryOption& option={})
   {
      std::string sql = "SELECT * FROM v_base_menu ";

      if ( option.limitOffsetValid() ) {
         sql += Helper::limitAndOffset(option.limit, option.offset, _sqlConn.dbmsName());
      }

      SqlQuery query(_sqlConn, sql);

      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         web::entity::MenuViewList items;

         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);
            items.emplace_back( getMenuView(sqlResult) );
         }

         return items;
      }

      return web::entity::MenuViewList();
   }

   web::entity::Menu getMenuById(const long long id)
   {
      std::string sql = "SELECT * FROM base_menu where id=:id";
      SqlQuery query(_sqlConn, sql);
      query.addParam("id", sql::DataType::bigint, id);

      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() == 1) {
         return getMenu(sqlResult);
      }

      return web::entity::Menu();
   }

   bool menuExists(const long long id)
   {
      std::string sql = "SELECT COUNT(*) FROM base_menu where id=:id";
      SqlQuery query(_sqlConn, sql);
      query.addParam("id", sql::DataType::bigint, id);
      auto result = query.executeScalar();
      return (result == "1");
   }

   bool deleteMenu(const long long id) 
   {
      try
      {
         std::string sql = "DELETE FROM base_menu WHERE id=:id";
         SqlQuery query(_sqlConn, sql);
         query.addParam("id", sql::DataType::bigint, id);
         bool success = query.executeVoid();
         return success;
      }
      catch(std::exception& ex)
      {
         Logger::logE("[webapp] UserAclDbRepo delete Menu, {}", ex.what());
      }

      return false;
   }

   bool insertMenu(const web::entity::Menu& menu)
   {
      try
      {
         std::string sql = R"-(
               INSERT INTO base_menu (name, group_id, type_id, pageurl, sidebar, methods)
                  VALUES ( :name, :group, :type, :url, :sidebar, :method) )-";
   
         SqlQuery query(_sqlConn, sql);
         query.addParam("name",    sql::DataType::varchar, menu.name);
         query.addParam("group",   sql::DataType::integer, menu.groupId);
         query.addParam("type",    sql::DataType::integer, menu.typeId);
         query.addParam("url",     sql::DataType::varchar, menu.pageurl);
         query.addParam("sidebar", sql::DataType::boolean, menu.sidebar);
         query.addParam("methods", sql::DataType::varchar, menu.methods);

         bool success = query.executeVoid();
         return success;
      }
      catch( const std::exception& ex)
      {
         Logger::logE("[webapp] UserAclDbRepo insert Menu, {}", ex.what());
      }

      return false;
   }

   bool updateMenu(const web::entity::Menu& menu)
   {
      try
      {
         std::string sql = R"-(
               UPDATE base_menu SET
               name     = :name,
               group_id = :group,
               type_id  = :type,
               pageurl  = :url,
               sidebar  = :sidebar,
               methods  = :methods
               WHERE id = :id )-";

         SqlQuery query(_sqlConn, sql);
         query.addParam("name",    sql::DataType::varchar, menu.name);
         query.addParam("group",   sql::DataType::integer, menu.groupId);
         query.addParam("type",    sql::DataType::integer, menu.typeId);
         query.addParam("url",     sql::DataType::varchar, menu.pageurl);
         query.addParam("sidebar", sql::DataType::boolean, menu.sidebar);
         query.addParam("methods", sql::DataType::varchar, menu.methods);
         query.addParam("id",      sql::DataType::bigint,  (long long)menu.id);

         bool success = query.executeVoid();
         return success;
      }
      catch( const std::exception& ex)
      {
         Logger::logE("[webapp] UserAclDbRepo edit Menu, {}", ex.what());
      }

      return false;
   }

   web::entity::MenuTypeViewList getMenuTypes()
   {
      std::string sql = "SELECT * FROM v_base_menu_type";
      SqlQuery query(_sqlConn, sql);

      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         web::entity::MenuTypeViewList items;
         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);
            web::entity::MenuTypeView item;

            item.id = sqlResult->getLongValue("id");
            item.name = sqlResult->getStringValue("name");
            item.code = sqlResult->getStringValue("code");

            items.push_back( item );
         }

         return items;
      }

      return web::entity::MenuTypeViewList();
   }

   web::entity::MenuGroupViewList getMenuGroups()
   {
      std::string sql = "SELECT * FROM v_base_menu_group ";
      SqlQuery query(_sqlConn, sql);

      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         web::entity::MenuGroupViewList items;
         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);
            web::entity::MenuGroupView item;

            item.id = sqlResult->getLongValue("id");
            item.name = sqlResult->getStringValue("name");
            item.code = sqlResult->getStringValue("code");

            items.push_back( item );
         }

         return items;
      }

      return web::entity::MenuGroupViewList();
   }


// -------------------------------------------------------
   web::entity::AclViewList getAcls(const sql::SqlQueryOption& option={}) 
   {
      std::string sql = "SELECT * FROM v_base_acl ";
      if ( option.limitOffsetValid() ) {
         sql += Helper::limitAndOffset(option.limit, option.offset, _sqlConn.dbmsName());
      }

      SqlQuery query(_sqlConn, sql);
      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         web::entity::AclViewList items;

         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);
            items.emplace_back( getAcl(sqlResult) );
         }

         return items;
      }

      return web::entity::AclViewList();
   }
   
   web::entity::AclView getAclById(const long long id)
   {
      std::string sql = "SELECT * FROM v_base_acl where id=:id";
      SqlQuery query(_sqlConn, sql);
      query.addParam("id", sql::DataType::bigint, id);

      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() == 1)
      {
         return getAcl(sqlResult);
      }

      return web::entity::AclView();
   }

   web::entity::AclView getAcl(std::shared_ptr<SqlResult> sqlResult)
   {
      web::entity::AclView acl;

      acl.id        = sqlResult->getLongValue("id");
      acl.ugId      = sqlResult->getLongValue("ug_id");
      acl.ugType    = sqlResult->getStringValue("ug_type");
      acl.ugName    = sqlResult->getStringValue("ug_name");
      acl.menuId    = sqlResult->getLongValue("menu_id");
      acl.menuName  = sqlResult->getStringValue("menu_name");
      acl.menuGroup = sqlResult->getStringValue("menu_group");
      acl.menuType  = sqlResult->getStringValue("menu_type");
      acl.pageUrl   = sqlResult->getStringValue("page_url");
      acl.aAll      = sqlResult->getBoolValue("a_all");
      acl.aIndex    = sqlResult->getBoolValue("a_index");
      acl.aAdd      = sqlResult->getBoolValue("a_add");
      acl.aDelete   = sqlResult->getBoolValue("a_delete");
      acl.aUpdate   = sqlResult->getBoolValue("a_update");
      acl.aPrint    = sqlResult->getBoolValue("a_print");
      acl.aOther    = sqlResult->getStringValue("a_other");

      return std::move(acl);
   }   

   bool aclExists(const long long id)
   {
      std::string sql = "SELECT COUNT(*) FROM base_acl where id=:id";
      SqlQuery query(_sqlConn, sql);
      query.addParam("id", sql::DataType::bigint, id);
      auto result = query.executeScalar();
      return (result == "1");
   }

   bool deleteAcl(const long long id)
   {
      try
      {
         std::string sql = "DELETE FROM base_acl WHERE id=:id";
         SqlQuery query(_sqlConn, sql);
         query.addParam("id", sql::DataType::bigint, id);
         bool success = query.executeVoid();
         return success;
      }
      catch(std::exception& ex)
      {
         Logger::logE("[webapp] UserAclDbRepo delete Acl, {}", ex.what());
      }

      return false;
   }

   bool insertAcl(const web::entity::Acl& acl)
   {
      try
      {
         std::string sql = R"-(
            INSERT INTO base_acl (ug_id, ug_type, menu_id, a_all,
               a_add, a_delete, a_update, a_print, a_index, a_other)
               VALUES ( :ugid, :ugtype, :menuid, :aall, :aadd, :adelete, :aupdate, :aprint, :aindex, :aother )-";

         SqlQuery query(_sqlConn, sql);
         query.addParam("ugid",    sql::DataType::integer, acl.ugId);
         query.addParam("ugtype",  sql::DataType::varchar, acl.ugType);
         query.addParam("menuid",  sql::DataType::integer, acl.menuId);
         query.addParam("aall",    sql::DataType::boolean, acl.aAll);
         query.addParam("aadd",    sql::DataType::boolean, acl.aAdd);
         query.addParam("adelete", sql::DataType::boolean, acl.aDelete);
         query.addParam("aupdate", sql::DataType::boolean, acl.aUpdate);
         query.addParam("aprint",  sql::DataType::boolean, acl.aPrint);
         query.addParam("aindex",  sql::DataType::boolean, acl.aIndex);
         query.addParam("aother",  sql::DataType::varchar, acl.aOther);

         bool success = query.executeVoid();
         return success;
      }
      catch( const std::exception& ex)
      {
         Logger::logE("[webapp] UserAclDbRepo insert Acl, {}", ex.what());
      }

      return false;
   }

   bool updateAcl(const web::entity::Acl& acl)
   {
      try
      {
         std::string sql = R"-(
            UPDATE base_acl SET
               ug_id    = :ugid,    ug_type  = :ugtype, menu_id  = :menuid,
               a_all    = :aall,    a_add    = :aadd,   a_delete = :adelete,
               a_update = :aupdate, a_print  = :aprint, a_index  = :aindex,
               a_other  = :aother   WHERE id = :id )-";

         SqlQuery query(_sqlConn, sql);
         query.addParam("ugid",    sql::DataType::integer, acl.ugId);
         query.addParam("ugtype",  sql::DataType::varchar, acl.ugType);
         query.addParam("menuid",  sql::DataType::integer, acl.menuId);
         query.addParam("aall",    sql::DataType::boolean, acl.aAll);
         query.addParam("aadd",    sql::DataType::boolean, acl.aAdd);
         query.addParam("adelete", sql::DataType::boolean, acl.aDelete);
         query.addParam("aupdate", sql::DataType::boolean, acl.aUpdate);
         query.addParam("aprint",  sql::DataType::boolean, acl.aPrint);
         query.addParam("aindex",  sql::DataType::boolean, acl.aIndex);
         query.addParam("aother",  sql::DataType::varchar, acl.aOther);
         query.addParam("id",      sql::DataType::bigint,  (long long)acl.id);

         bool success = query.executeVoid();
         return success;
      }
      catch( const std::exception& ex)
      {
         Logger::logE("[webapp] UserAclDbRepo update Acl, {}", ex.what());
      }

      return false;
   }

};

} // namespace app
} // namespace tbs
