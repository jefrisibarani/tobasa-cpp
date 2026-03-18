#pragma once

#if (defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)) || defined(TOBASA_SQL_USE_ODBC)

#include <string>
#include <vector>

namespace tbs {
namespace dbm {
namespace mssql {

const std::string t_base_users( R"-(
   CREATE TABLE base_users (
      id            int IDENTITY(1,1) PRIMARY KEY,
      uuid          varchar(64)     NOT NULL UNIQUE,
      user_name     varchar(100)    NOT NULL UNIQUE,
      first_name    varchar(255)    NOT NULL,
      last_name     varchar(255)    NOT NULL,
      email         varchar(255)    NOT NULL,
      image         varchar(255)    NOT NULL DEFAULT '',
      enabled       bit             NOT NULL DEFAULT 1,
      password_salt varbinary(max)  NOT NULL,
      password_hash varbinary(max)  NOT NULL,
      allow_login   bit             NOT NULL DEFAULT 1,
      created       datetime        NOT NULL DEFAULT getdate(),
      updated       datetime        NOT NULL DEFAULT getdate(),
      expired       datetime        NOT NULL DEFAULT dateadd(year,(1),getdate()),
      last_login    datetime2(7)    NULL,
      unique_code   varchar(48)     NOT NULL DEFAULT '',
      birth_date    date            NOT NULL,
      phone         varchar(20)     NOT NULL DEFAULT '',
      gender        varchar(1)      NOT NULL DEFAULT '',
      address       varchar(512)    NOT NULL DEFAULT '',
      nik           varchar(16)     NOT NULL DEFAULT ''
   );
)-");

const std::string t_base_roles( R"-(
   CREATE TABLE base_roles (
      id       int IDENTITY(1,1) PRIMARY KEY,
      name     varchar(50)    NOT NULL,
      alias    varchar(100)   NOT NULL,
      enabled  bit            NOT NULL DEFAULT 1,
      created  datetime       NOT NULL DEFAULT getdate(),
      updated  datetime       NOT NULL DEFAULT getdate(),
      sysrole  bit            NOT NULL DEFAULT 0
   );
)-");

const std::string t_base_sites( R"-(
   CREATE TABLE base_sites (
      id       int IDENTITY(1,1) PRIMARY KEY,
      code     varchar(7)     NOT NULL DEFAULT '',
      name     varchar(50)    NOT NULL DEFAULT '',
      address  varchar(512)   NOT NULL DEFAULT ''
   );
)-");

const std::string t_base_user_role( R"-(
   CREATE TABLE base_user_role (
      id          int IDENTITY(1,1) NOT NULL,
      user_id     int   NOT NULL,
      role_id     int   NOT NULL,
      is_primary  bit   NOT NULL,
      CONSTRAINT PK_base_user_role PRIMARY KEY CLUSTERED
      (
         user_id ASC,
         role_id ASC
      ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
   ) ON [PRIMARY];

   ALTER TABLE base_user_role ADD  CONSTRAINT DF_base_user_role_primary  DEFAULT ((0)) FOR is_primary;

   ALTER TABLE base_user_role  WITH CHECK ADD  CONSTRAINT FK_base_user_role_base_user_role FOREIGN KEY(role_id)
   REFERENCES base_roles (id)
   ON UPDATE CASCADE ON DELETE CASCADE;

   ALTER TABLE base_user_role CHECK CONSTRAINT FK_base_user_role_base_user_role;

   ALTER TABLE base_user_role  WITH CHECK ADD  CONSTRAINT FK_base_user_role_base_users FOREIGN KEY(user_id)
   REFERENCES base_users (id)
   ON UPDATE CASCADE ON DELETE CASCADE;

   ALTER TABLE base_user_role CHECK CONSTRAINT FK_base_user_role_base_users;
)-");

const std::string t_base_user_site( R"-(
   CREATE TABLE base_user_site (
      id          int IDENTITY(1,1) NOT NULL,
      user_id     int NOT NULL,
      site_id     int NOT NULL,
      allow_login bit NOT NULL,
      is_admin    bit NOT NULL,
      CONSTRAINT PK_base_user_site PRIMARY KEY CLUSTERED
      (
         user_id ASC,
         site_id ASC
      ) WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
   ) ON [PRIMARY];

   ALTER TABLE base_user_site ADD  CONSTRAINT DF_base_user_site_allow_login  DEFAULT ((1)) FOR allow_login;
   ALTER TABLE base_user_site ADD  CONSTRAINT DF_base_user_site_is_admin  DEFAULT ((0)) FOR is_admin;
   ALTER TABLE base_user_site  WITH CHECK ADD  CONSTRAINT FK_base_user_site_base_sites FOREIGN KEY(site_id)
   REFERENCES base_sites (id)
   ON UPDATE CASCADE ON DELETE CASCADE;

   ALTER TABLE base_user_site CHECK CONSTRAINT FK_base_user_site_base_sites;

   ALTER TABLE base_user_site  WITH CHECK ADD  CONSTRAINT FK_base_user_site_base_users FOREIGN KEY(user_id)
   REFERENCES base_users (id)
   ON UPDATE CASCADE ON DELETE CASCADE;

   ALTER TABLE base_user_site CHECK CONSTRAINT FK_base_user_site_base_users;
)-");

const std::string t_base_users_reset_password( R"-(
   CREATE TABLE base_users_reset_password (
      id             int IDENTITY(1,1) PRIMARY KEY,
      user_id        int         NOT NULL,
      request_time   datetime    NOT NULL DEFAULT getdate(),
      expired_time   bigint      NOT NULL,
      reset_code     varchar(64) NOT NULL,
      success        bit         NOT NULL DEFAULT 0,
      success_time   datetime    NOT NULL DEFAULT getdate()
   );
)-");


const std::string t_base_acl( R"-(
   CREATE TABLE base_acl (
      id       int IDENTITY(1,1) PRIMARY KEY,
      ug_id    int            NOT NULL,
      ug_type  varchar(1)     NOT NULL DEFAULT 'G',
      menu_id  int            NOT NULL,
      a_all    bit            NOT NULL DEFAULT 1,
      a_add    bit            NOT NULL DEFAULT 1,
      a_delete bit            NOT NULL DEFAULT 1,
      a_update bit            NOT NULL DEFAULT 1,
      a_print  bit            NOT NULL DEFAULT 1,
      a_index  bit            NOT NULL DEFAULT 1,
      a_other  varchar(1024)  NULL DEFAULT ''
   );

   ALTER TABLE base_acl WITH CHECK ADD  CONSTRAINT CHK_valid_ug_Type CHECK  ((ug_type='G' OR ug_type='U'));
   ALTER TABLE base_acl CHECK CONSTRAINT CHK_valid_ug_Type;
)-");

const std::string t_base_auth_log( R"-(
   CREATE TABLE base_auth_log (
      id          int IDENTITY(1,1) PRIMARY KEY,
      logon_time  datetime2(7)   NOT NULL DEFAULT getdate(),
      usr_id      int            NOT NULL,
      usr_name    nvarchar(255)  NOT NULL,
      text_note   varchar(255)   NOT NULL DEFAULT '',
      src_ip      varchar(255)   NOT NULL DEFAULT '',
      src_host    varchar(255)   NOT NULL DEFAULT '',
      src_mac     varchar(100)   NOT NULL DEFAULT '',
      auth_type   varchar(30)    NOT NULL DEFAULT '',
      site_id     int            NOT NULL,
      logged_out  bit            NULL
   );
)-");

const std::string t_base_class_code( R"-(
   CREATE TABLE base_class_code (
      id          int IDENTITY(1,1) PRIMARY KEY,
      item_name   varchar(255)   NOT NULL,
      item_class  varchar(255)   NOT NULL,
      item_id     int            NOT NULL,
      item_code   nchar(10)      NOT NULL
   );
)-");

const std::string t_base_menu( R"-(
   CREATE TABLE base_menu (
      id       int IDENTITY(1,1) PRIMARY KEY,
      name     varchar(255)   NOT NULL,
      group_id int            NOT NULL,
      type_id  int            NOT NULL,
      pageurl  varchar(255)   NOT NULL,
      sidebar  bit            NOT NULL DEFAULT 0,
      methods  varchar(512)   NOT NULL DEFAULT ''
   );
)-");

const std::string t_company( R"-(
   CREATE TABLE company (
      id       int IDENTITY(1,1) PRIMARY KEY,
      name     varchar(255) NOT NULL DEFAULT '',
      address  varchar(512) NOT NULL DEFAULT '',
      phone    varchar(15)  NOT NULL DEFAULT '',
      email    varchar(200) NOT NULL DEFAULT '',
      website  varchar(200) NOT NULL DEFAULT ''
   );
)-");

const std::string t_base_event_log( R"-(
   CREATE TABLE base_event_log (
      id          bigint IDENTITY(1,1) PRIMARY KEY,
      event_time  datetime2      NOT NULL DEFAULT getdate(),
      src_module  varchar(100)   NOT NULL DEFAULT '',
      usr_id      int            NOT NULL,
      usr_name    varchar(255)   NOT NULL,
      text_note   varchar(512)   NOT NULL DEFAULT ''
   );
)-");

const std::string t_base_app_task( R"-(
   CREATE TABLE base_app_task (
      id             bigint IDENTITY(1,1) PRIMARY KEY,
      task_id        integer        NOT NULL,
      name           varchar(100)   NOT NULL DEFAULT '',
      info           varchar(256)   NOT NULL DEFAULT '',
      status         varchar(100)   NOT NULL DEFAULT '',
      result_status  varchar(256)   NOT NULL DEFAULT '',
      result_message varchar(256)   NOT NULL DEFAULT '',
      result_code    integer        NOT NULL,
      start_time     varchar(100)   NOT NULL DEFAULT '',
      end_time       varchar(100)   NOT NULL DEFAULT '',
      duration       varchar(100)   NOT NULL DEFAULT '',
      user_id        varchar(100)   NOT NULL DEFAULT '',
      app_module     varchar(100)   NOT NULL DEFAULT ''
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
      ('Home_Resource', 1, 1, '/resource/{resource_type}/{fileName}', 0, 'get')
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
   SET IDENTITY_INSERT base_roles ON;
   INSERT INTO base_roles (id, name, alias, enabled, sysrole) VALUES
      (1, 'role_admin', 'Admin',    1, 1),
      (2, 'role_user',  'User',     1, 1),
      (3, 'role_app',   'App User', 1, 1);
   SET IDENTITY_INSERT base_roles OFF;
)-");

// First Admin user. Password is AdmBaru9
const std::string insert_first_admin_user( R"-(
   SET IDENTITY_INSERT base_users ON;
   INSERT INTO base_users (id, uuid, user_name, first_name, last_name, email, image, enabled, password_salt, password_hash,
                           allow_login, expired, unique_code, birth_date, phone, gender, address, nik )
      VALUES ( 1, '-CldWkC8-Gje4O9TKbhd', 'admin', 'Administrator', 'Administrator', 'admin@mangapul.net', '', 1,
               0x8420B2E991C17F6C102E4061F77B9DB914E9069C119B9CB159304C9BEAFFBE41,
               0x2632E5FBEE65C30D045C4A4FD9FDC971424A2B93A521D99E28DF855BD63687EA4293B3679E67E61D9E24C58475E4C6400AF01136C895D125767474734971D2A5,
               1, (dateadd(year,(1),getdate())), '121211', '2000-01-01', '121212121211', 'M', 'JAKARTA', '1212121212121211' );
   SET IDENTITY_INSERT base_users OFF;
)-");


// First Sys user. Password is AdmUser9
const std::string insert_first_sys_user( R"-(
   SET IDENTITY_INSERT base_users ON;
   INSERT INTO base_users (id, uuid, user_name, first_name, last_name, email, image, enabled, password_salt, password_hash,
                           allow_login, expired, unique_code, birth_date, phone, gender, address, nik )
      VALUES ( 3, '-DgT2vS8FYI1sENOkAlY', 'user1', 'User', 'User', 'user@mangapul.net', '', 1,
               0x247C5E834C87510639C78AD0999882EFD17EC8F7E04EC228AE1EF3AA9F450B91,
               0x154E408F4B4C7A1F6201561BD37877ABF7DB5FD2B7C3D3ABF1256C1321AE1335BD5723D990FFD06CE5473F51C49BC760DB4D6D694F77581F3ACAD44A37889CAD,
               1, (dateadd(year,(1),getdate())), '121213', '2000-01-01', '121212121213', 'M', 'JAKARTA', '1212121212121213' );
   SET IDENTITY_INSERT base_users OFF;
)-");

const std::string insert_base_user_role( R"-(
   SET IDENTITY_INSERT base_user_role ON;
   INSERT INTO base_user_role (id, user_id, role_id, is_primary) 
   VALUES (1, 1, 1, 1),  (3, 3, 2, 1);
   SET IDENTITY_INSERT base_user_role OFF;
)-");

const std::string insert_base_user_site( R"-(
   SET IDENTITY_INSERT base_user_site ON;
   INSERT INTO base_user_site (id, user_id, site_id, allow_login, is_admin) 
   VALUES (1, 1, 1, 1, 1),  (3, 3, 1, 1, 0);
   SET IDENTITY_INSERT base_user_site OFF;
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

} // namespace mssql
} // namespace dbm
} // namespace tbs

#endif // (defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)) || defined(TOBASA_SQL_USE_ODBC)