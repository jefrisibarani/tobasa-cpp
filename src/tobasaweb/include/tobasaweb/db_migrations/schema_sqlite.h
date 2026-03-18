#pragma once

#if defined(TOBASA_SQL_USE_SQLITE)

#include <string>
#include <vector>

namespace tbs {
namespace dbm {
namespace sqlite {

const std::string t_base_users( R"-(
   CREATE TABLE base_users (
      id             INTEGER PRIMARY KEY AUTOINCREMENT,
      uuid           VARCHAR(64)    NOT NULL UNIQUE,
      user_name      VARCHAR(50)    NOT NULL UNIQUE,
      first_name     VARCHAR(127)   NOT NULL,
      last_name      VARCHAR(127)   NOT NULL,
      email          VARCHAR(256)   NOT NULL,
      image          VARCHAR(256)   NOT NULL DEFAULT '',
      enabled        TINYINT        NOT NULL DEFAULT 1 CHECK (enabled IN (0, 1)),
      password_salt  BLOB           NOT NULL,
      password_hash  BLOB           NOT NULL,
      allow_login    TINYINT(1)     NOT NULL DEFAULT '1',
      created        DATETIME       NOT NULL DEFAULT CURRENT_TIMESTAMP,
      updated        DATETIME       NOT NULL DEFAULT CURRENT_TIMESTAMP,
      expired        DATETIME       NOT NULL DEFAULT '2024-12-31 00:00:00',
      last_login     DATETIME       NULL,
      unique_code    VARCHAR(48)    NULL DEFAULT '',
      birth_date     DATE           NOT NULL,
      phone          VARCHAR(20)    NULL DEFAULT '',
      gender         VARCHAR(1)     NULL DEFAULT '',
      address        VARCHAR(512)   NOT NULL DEFAULT '',
      nik            VARCHAR(16)    NOT NULL DEFAULT ''
   );
)-");

const std::string t_base_roles( R"-(
   CREATE TABLE base_roles (
      id       INTEGER PRIMARY KEY AUTOINCREMENT,
      name     VARCHAR(50)    NOT NULL UNIQUE,
      alias    VARCHAR(100)   NOT NULL UNIQUE,
      enabled  TINYINT        NOT NULL DEFAULT 0 CHECK (enabled IN (0, 1)),
      created  DATETIME       NOT NULL DEFAULT CURRENT_TIMESTAMP,
      updated  DATETIME       NOT NULL DEFAULT CURRENT_TIMESTAMP,
      sysrole  TINYINT        NOT NULL DEFAULT 0 CHECK (enabled IN (0, 1))
   );
)-");

const std::string t_base_sites( R"-(
   CREATE TABLE base_sites (
      id       INTEGER PRIMARY KEY AUTOINCREMENT,
      code     VARCHAR(7)     NOT NULL UNIQUE,
      name     VARCHAR(50)    NOT NULL UNIQUE,
      address  VARCHAR(512)   NOT NULL DEFAULT ''
   );
)-");

const std::string t_base_user_role( R"-(
   CREATE TABLE base_user_role (
      id          INTEGER PRIMARY KEY AUTOINCREMENT,
      user_id     INTEGER NOT NULL,
      role_id     INTEGER NOT NULL,
      is_primary  TINYINT NOT NULL DEFAULT 0 CHECK (is_primary IN (0, 1)),
      FOREIGN KEY (role_id) REFERENCES base_roles(id) ON UPDATE CASCADE ON DELETE CASCADE,
      FOREIGN KEY (user_id) REFERENCES base_users(id) ON UPDATE CASCADE ON DELETE CASCADE,
      UNIQUE(user_id, role_id)
   );
)-");

const std::string t_base_user_site( R"-(
   CREATE TABLE base_user_site (
      id          INTEGER PRIMARY KEY AUTOINCREMENT,
      user_id     INTEGER NOT NULL,
      site_id     INTEGER NOT NULL,
      allow_login TINYINT NOT NULL DEFAULT 1 CHECK (allow_login IN (0, 1)),
      is_admin    TINYINT NOT NULL DEFAULT 0 CHECK (is_admin IN (0, 1)),
      FOREIGN KEY (site_id) REFERENCES base_sites(id) ON UPDATE CASCADE ON DELETE CASCADE,
      FOREIGN KEY (user_id) REFERENCES base_users(id) ON UPDATE CASCADE ON DELETE CASCADE,
      UNIQUE (user_id, site_id)
   );
)-");

const std::string t_base_users_reset_password( R"-(
   CREATE TABLE base_users_reset_password (
      id           INTEGER PRIMARY KEY AUTOINCREMENT,
      user_id      INTEGER      NOT NULL,
      request_time DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
      expired_time BIGINT       NOT NULL,
      reset_code   VARCHAR(255) NOT NULL,
      success      TINYINT      NOT NULL DEFAULT 0 CHECK (success IN (0, 1)),
      success_time DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP
   );
)-");


const std::string t_base_acl( R"-(
   CREATE TABLE base_acl (
      id       INTEGER PRIMARY KEY AUTOINCREMENT,
      ug_id    INTEGER        NOT NULL,
      ug_type  VARCHAR(1)     NOT NULL DEFAULT 'G',
      menu_id  INTEGER        NOT NULL,
      a_all    TINYINT        NOT NULL DEFAULT 1 CHECK (a_all IN (0, 1)),
      a_add    TINYINT        NOT NULL DEFAULT 1 CHECK (a_add IN (0, 1)),
      a_delete TINYINT        NOT NULL DEFAULT 1 CHECK (a_delete IN (0, 1)),
      a_update TINYINT        NOT NULL DEFAULT 1 CHECK (a_update IN (0, 1)),
      a_print  TINYINT        NOT NULL DEFAULT 1 CHECK (a_print IN (0, 1)),
      a_index  TINYINT        NOT NULL DEFAULT 1 CHECK (a_index IN (0, 1)),
      a_other  VARCHAR(1024)  DEFAULT '',
      UNIQUE (ug_id, ug_type, menu_id)
   );
)-");

const std::string t_base_auth_log( R"-(
   CREATE TABLE base_auth_log (
      id          INTEGER PRIMARY KEY AUTOINCREMENT,
      logon_time  DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
      usr_id      INTEGER      NOT NULL,
      usr_name    VARCHAR(255) NOT NULL,
      text_note   VARCHAR(255) NOT NULL DEFAULT '',
      src_ip      VARCHAR(255) NOT NULL DEFAULT '',
      src_host    VARCHAR(255) NOT NULL DEFAULT '',
      src_mac     VARCHAR(100) NOT NULL DEFAULT '',
      auth_type   VARCHAR(30)  NOT NULL DEFAULT '',
      site_id     INTEGER      NOT NULL,
      logged_out  TINYINT      NULL
   );
)-");

const std::string t_base_class_code( R"-(
   CREATE TABLE base_class_code (
      id          INTEGER PRIMARY KEY AUTOINCREMENT,
      item_name   VARCHAR NOT NULL,
      item_class  VARCHAR NOT NULL,
      item_id     INTEGER NOT NULL,
      item_code   VARCHAR NOT NULL
    );
)-");

const std::string t_base_menu( R"-(
   CREATE TABLE base_menu (
      id       INTEGER PRIMARY KEY AUTOINCREMENT,
      name     VARCHAR(255) NOT NULL UNIQUE,
      group_id INTEGER      NOT NULL,
      type_id  INTEGER      NOT NULL,
      pageurl  VARCHAR(255) NOT NULL,
      sidebar  TINYINT      NOT NULL DEFAULT 1 CHECK (sidebar IN (0, 1)),
      methods  VARCHAR(512) NOT NULL DEFAULT ''
   ) ;
)-");

const std::string t_company( R"-(
   CREATE TABLE company (
      id       INTEGER PRIMARY KEY AUTOINCREMENT,
      name     VARCHAR(200)   NOT NULL DEFAULT '',
      address  VARCHAR(512)   NOT NULL DEFAULT '',
      phone    VARCHAR(15)    NOT NULL DEFAULT '',
      email    VARCHAR(200)   NOT NULL DEFAULT '',
      website  VARCHAR(200)   NOT NULL DEFAULT ''
   );
)-");

const std::string t_base_event_log( R"-(
   CREATE TABLE base_event_log (
      id          INTEGER PRIMARY KEY AUTOINCREMENT,
      event_time  DATETIME NOT   NULL DEFAULT CURRENT_TIMESTAMP,
      src_module  VARCHAR(100)   NOT NULL DEFAULT '',
      usr_id      INTEGER        NOT NULL,
      usr_name    VARCHAR(255)   NOT NULL,
      text_note   VARCHAR(512)   NOT NULL DEFAULT ''
   );
)-");

const std::string t_base_app_task( R"-(
   CREATE TABLE base_app_task (
      id             INTEGER PRIMARY KEY AUTOINCREMENT,
      task_id        INTEGER        NOT NULL,
      name           VARCHAR(100)   NOT NULL DEFAULT '',
      info           VARCHAR(256)   NOT NULL DEFAULT '',
      status         VARCHAR(100)   NOT NULL DEFAULT '',
      result_status  VARCHAR(256)   NOT NULL DEFAULT '',
      result_message VARCHAR(256)   NOT NULL DEFAULT '',
      result_code    INTEGER        NOT NULL,
      start_time     VARCHAR(100)   NOT NULL DEFAULT '',
      end_time       VARCHAR(100)   NOT NULL DEFAULT '',
      duration       VARCHAR(100)   NOT NULL DEFAULT '',
      user_id        VARCHAR(100)   NOT NULL DEFAULT '',
      app_module     VARCHAR(100)   NOT NULL DEFAULT ''
   );
)-");


// -------------------------------------------------------

const std::string v_base_acl( R"-(
   CREATE VIEW v_base_acl AS
   SELECT a.id, a.ug_id,
      CASE a.ug_type
            WHEN 'G' THEN 'Group'
            WHEN 'U' THEN 'User'
            ELSE NULL
      END AS ug_type,
      CASE a.ug_type
            WHEN 'G' THEN ( SELECT r.name FROM base_roles r WHERE r.id = a.ug_id)
            WHEN 'U' THEN ( SELECT u.user_name FROM base_users u WHERE u.id = a.ug_id)
            ELSE NULL
      END AS ug_name,
      m.id AS menu_id,
      m.name AS menu_name,
      ( SELECT cc.item_code FROM base_class_code cc
            WHERE cc.item_id = m.group_id AND cc.item_class = 'MENU_GROUP') AS menu_group,
      ( SELECT cc.item_code FROM base_class_code cc
            WHERE cc.item_id = m.type_id AND cc.item_class = 'MENU_TYPE') AS menu_type,
      m.pageurl AS page_url,
      a.a_all, a.a_index, a.a_add, a.a_delete, a.a_update, a.a_print, a.a_other
   FROM base_acl a
   LEFT JOIN base_menu m ON m.id = a.menu_id;
)-");

const std::string v_base_menu_group( R"-(
   CREATE VIEW v_base_menu_group AS
   SELECT cc.item_id AS id, cc.item_name AS name, cc.item_code AS code
   FROM base_class_code cc
   WHERE cc.item_class = 'MENU_GROUP';
)-");

const std::string v_base_menu_type( R"-(
   CREATE VIEW v_base_menu_type AS
   SELECT cc.item_id AS id, cc.item_name AS name, cc.item_code AS code
   FROM base_class_code cc
   WHERE cc.item_class = 'MENU_TYPE';
)-");

const std::string v_base_menu( R"-(
   CREATE VIEW v_base_menu AS
   SELECT m.id, m.name,
      ( SELECT cc.item_code FROM base_class_code cc
            WHERE cc.item_id = m.type_id AND cc.item_class = 'MENU_TYPE') AS menu_type,
      ( SELECT cc.item_code FROM base_class_code cc
            WHERE cc.item_id = m.group_id AND cc.item_class = 'MENU_GROUP') AS menu_group,
      m.pageurl, m.sidebar, m.methods
   FROM base_menu m;
)-");

const std::string v_base_user_roles( R"-(
   CREATE VIEW v_base_user_roles AS
   SELECT ur.id, ur.user_id, u.user_name, ur.role_id, r.name AS role_name
   FROM base_user_role ur
   JOIN base_users u ON u.id = ur.user_id
   JOIN base_roles r ON r.id = ur.role_id;
)-");

const std::string v_base_user_site( R"-(
   CREATE VIEW v_base_user_site AS
   SELECT us.id, us.user_id, u.user_name, us.site_id, s.name AS site_name
   FROM base_user_site us
   JOIN base_users u ON u.id = us.user_id
   JOIN base_sites s ON s.id = us.site_id;
)-");

// -------------------------------------------------------

const std::string insert_base_class_code( R"-(
   INSERT INTO base_class_code (item_name, item_class, item_id, item_code) VALUES
      ('User',   'ROLE',       1, 'U'),
      ('Role',   'ROLE',       2, 'G'),
      ('Page',   'MENU_TYPE',  1, 'PAGE'),
      ('API',    'MENU_TYPE',  2, 'API'),
      ('Module', 'MENU_TYPE',  3, 'MODULE'),
      ('Base',   'MENU_GROUP', 1, 'BASE'),
      ('Admin',  'MENU_GROUP', 2, 'ADMIN'),
      ('Master', 'MENU_GROUP', 3, 'MASTER'),
      ('Report', 'MENU_GROUP', 4, 'REPORT')
)-");

const std::string insert_base_menu( R"-(
   INSERT INTO base_menu (name, group_id, type_id, pageurl, sidebar, methods) VALUES
      ('API_Server_Status', 1, 2, '/api/server_status', 0, 'get'),
      ('API_Authenticate', 1, 2, '/api/authenticate', 0, 'post'),
      ('API_Decrypt', 1, 2, '/api/decrypt', 0, 'get'),
      ('API_Encrypt', 1, 2, '/api/encrypt', 0, 'get'),
      ('API_Read_log_size_source', 1, 2, '/api/read_log/{size}/{source}', 0, 'get'),
      ('API_Version', 1, 2, '/api/version', 0, 'get'),
      ('API_Users', 1, 2, '/api/users', 0, 'get'),
      ('API_Users_register', 1, 2, '/api/users/register', 0, 'post'),
      ('API_Users_register_with_image', 1, 2, '/api/users/register_with_image', 0, 'post'),
      ('API_Users_authenticate', 1, 2, '/api/users/authenticate', 0, 'post'),
      ('API_Users_change_password', 1, 2, '/api/users/change_password', 0, 'post'),
      ('API_Users_check_password', 1, 2, '/api/users/check_password', 0, 'post'),
      ('API_Users_delete', 1, 2, '/api/users/delete', 0, 'delete'),
      ('API_Users_forgot_password', 1, 2, '/api/users/forgot_password', 0, 'post'),
      ('API_Users_id', 1, 2, '/api/users/{user_id}', 0, 'get'),
      ('API_Users_exists_user_name', 1, 2, '/api/users/exists/{user_name}', 0, 'get'),
      ('API_Users_reset_password', 1, 2, '/api/users/reset_password', 0, 'post'),
      ('API_Users_update_profile', 1, 2, '/api/users/update_profile', 0, 'put'),
      ('API_Users_update_profile_with_mage', 1, 2, '/api/users/update_profile_with_image', 0, 'post'),
      ('API_Users_user_id_profile_image', 1, 2, '/api/users/{user_id}/profile_image', 0, 'get'),
      ('API_Users_user_id_roles', 1, 2, '/api/users/{user_id}/roles', 0, 'get'),
      ('API_Admin_ACL', 2, 2, '/api/admin/acl', 0, 'post,delete'),
      ('API_Admin_Menus', 2, 2, '/api/admin/menus', 0, 'post,delete'),
      ('API_Admin_Roles', 2, 2, '/api/admin/roles', 0, 'post,delete'),
      ('API_Admin_Roles_addusers', 2, 2, '/api/admin/roles/addusers', 0, 'post'),
      ('API_Admin_Roles_get_member', 2, 2, '/api/admin/roles/get_member', 0, 'get'),
      ('API_Admin_Roles_get_non_member', 2, 2, '/api/admin/roles/get_non_member', 0, 'get'),
      ('API_Admin_Roles_remove_user', 2, 2, '/api/admin/roles/remove_user', 0, 'post'),
      ('API_Admin_Users', 2, 2, '/api/admin/users', 0, 'post,delete'),
      ('API_Admin_Users_reset_password', 2, 2, '/api/admin/users/reset_password', 0, 'post'),
      ('Admin', 2, 1, '/admin', 1, 'get'),
      ('Admin_ACL', 2, 1, '/admin/acl', 1, 'get'),
      ('Admin_Menus', 2, 1, '/admin/menus', 1, 'get'),
      ('Admin_Roles', 2, 1, '/admin/roles', 1, 'get'),
      ('Admin_Users', 2, 1, '/admin/users', 1, 'get'),
      ('Home_About', 1, 1, '/about', 1, 'all'),
      ('Home_Server_status', 1, 1, '/server_status', 1, 'get'),
      ('Home_Dashboard', 1, 1, '/dashboard', 1, 'get'),
      ('Home_Keep_alive', 1, 1, '/keep_alive', 0, 'get'),
      ('Home_Login', 1, 1, '/login', 1, 'get,post'),
      ('Home_Logout', 1, 1, '/logout', 1, 'get'),
      ('Home_Page', 1, 1, '/', 1, 'get'),
      ('Home_Password', 1, 1, '/password', 1, 'get,post'),
      ('Home_Privacy', 1, 1, '/privacy', 1, 'all'),
      ('Home_Register', 1, 1, '/register', 1, 'get,post'),
      ('Home_Status_page', 1, 1, '/spage', 1, 'get'),
      ('Home_User_profile', 1, 1, '/user_profile', 0, 'get'),
      ('Home_Resource', 1, 1, '/resource/{resource_type}/{fileName}', 0, 'get');
)-");

const std::string insert_company( R"-(
   INSERT INTO company (name, address, phone, email, website) VALUES
      ('TOBASA', 'Jakarta', '333333', 'admin@mangapul.net', 'www.mangapul.net');
)-");

const std::string insert_base_sites( R"-(
   INSERT INTO base_sites (code, name, address) VALUES
      ('S001', 'SITE 1', 'JAKARTA'),
      ('S002', 'SITE 2', 'BANDUNG'),
      ('S003', 'SITE 3', 'MEDAN');
)-");

const std::string insert_base_roles( R"-(
   INSERT INTO base_roles (id, name, alias, enabled, sysrole) VALUES
      (1, 'role_admin', 'Admin',    true, true),
      (2, 'role_user',  'User',     true, true),
      (3, 'role_app',   'App User', true, true);
)-");

// First Admin user. Password is AdmBaru9
const std::string insert_first_admin_user( R"-(
   INSERT INTO base_users (id, uuid, user_name, first_name, last_name, email, image, enabled, password_salt, password_hash,
                           allow_login, expired, unique_code, birth_date, phone, gender, address, nik )
      VALUES ( 1, '-CldWkC8-Gje4O9TKbhd', 'admin', 'Administrator', 'Administrator', 'admin@mangapul.net', '', true,
               '8420B2E991C17F6C102E4061F77B9DB914E9069C119B9CB159304C9BEAFFBE41',
               '2632E5FBEE65C30D045C4A4FD9FDC971424A2B93A521D99E28DF855BD63687EA4293B3679E67E61D9E24C58475E4C6400AF01136C895D125767474734971D2A5',
               true, DATETIME(CURRENT_TIMESTAMP, '+1 years'), '121211', '2000-01-01', '121212121211', 'M', 'JAKARTA', '1212121212121211' );
)-");

// First Sys user. Password is AdmUser9
const std::string insert_first_sys_user( R"-(
   INSERT INTO base_users (id, uuid, user_name, first_name, last_name, email, image, enabled, password_salt, password_hash,
                           allow_login, expired, unique_code, birth_date, phone, gender, address, nik )
      VALUES ( 3, '-DgT2vS8FYI1sENOkAlY', 'user1', 'User', 'User', 'user@mangapul.net', '', true,
               '247C5E834C87510639C78AD0999882EFD17EC8F7E04EC228AE1EF3AA9F450B91',
               '154E408F4B4C7A1F6201561BD37877ABF7DB5FD2B7C3D3ABF1256C1321AE1335BD5723D990FFD06CE5473F51C49BC760DB4D6D694F77581F3ACAD44A37889CAD',
               true, DATETIME(CURRENT_TIMESTAMP, '+1 years'), '121213', '2000-01-01', '121212121213', 'M', 'JAKARTA', '1212121212121213' );
)-");

const std::string insert_base_user_role( R"-(
   INSERT INTO base_user_role (id, user_id, role_id, is_primary) 
   VALUES (1, 1, 1, true),  (3, 3, 2, true);
)-");

const std::string insert_base_user_site( R"-(
   INSERT INTO base_user_site (id, user_id, site_id, allow_login, is_admin) 
   VALUES (1, 1, 1, true, true),  (3, 3, 1, true, false);
)-");

inline std::vector<std::string> getQueries()
{
   std::vector<std::string> queries;

   queries.push_back(t_base_users);
   queries.push_back(t_base_roles);
   queries.push_back(t_base_sites);
   queries.push_back(t_base_user_role);
   queries.push_back(t_base_user_site);
   queries.push_back(t_base_users_reset_password);
   queries.push_back(t_base_acl);
   queries.push_back(t_base_auth_log);
   queries.push_back(t_base_class_code);
   queries.push_back(t_base_menu);
   queries.push_back(t_company);
   queries.push_back(t_base_event_log);
   queries.push_back(t_base_app_task);

   queries.push_back(v_base_acl);
   queries.push_back(v_base_menu_group);
   queries.push_back(v_base_menu_type);
   queries.push_back(v_base_menu);
   queries.push_back(v_base_user_roles);
   queries.push_back(v_base_user_site);

   queries.push_back(insert_base_class_code);
   queries.push_back(insert_base_menu);
   queries.push_back(insert_company);
   queries.push_back(insert_base_sites);
   queries.push_back(insert_base_roles);
   queries.push_back(insert_first_admin_user);
   queries.push_back(insert_first_sys_user);
   queries.push_back(insert_base_user_role);
   queries.push_back(insert_base_user_site);

   return queries;
}

} // namespace sqlite
} // namespace dbm
} // namespace tbs

#endif //defined(TOBASA_SQL_USE_SQLITE)