#pragma once

#if defined(TOBASA_SQL_USE_PGSQL)

#include <string>
#include <vector>

namespace tbs {
namespace dbm {
namespace pgsql {

// Note: Postgresql: The SQL standard requires that writing just timestamp be equivalent to timestamp without time zone, 
// and PostgreSQL honors that behavior. timestamptz is accepted as an abbreviation for timestamp with time zone; this is a PostgreSQL extension   

const std::string t_base_users( R"-(
   CREATE TABLE public.base_users (
      id             serial PRIMARY KEY,
      uuid           varchar(64)    NOT NULL UNIQUE,
      user_name      varchar(100)   NOT NULL UNIQUE,
      first_name     varchar(255)   NOT NULL,
      last_name      varchar(255)   NOT NULL,
      email          varchar(255)   NOT NULL,
      image          varchar(255)   NOT NULL DEFAULT '',
      enabled        boolean        NOT NULL DEFAULT true,
      password_salt  bytea          NOT NULL,
      password_hash  bytea          NOT NULL,
      allow_login    boolean        NOT NULL DEFAULT true,
      created        timestamp      NOT NULL DEFAULT now(),
      updated        timestamp      NOT NULL DEFAULT now(),
      expired        timestamp      NOT NULL DEFAULT (now() + '1 year'::interval),
      last_login     timestamp,
      unique_code    varchar(48)    NOT NULL DEFAULT '',
      birth_date     date           NOT NULL,
      phone          varchar(20)    NOT NULL DEFAULT '',
      gender         varchar(1)     NOT NULL DEFAULT '',
      address        varchar(512)   NOT NULL DEFAULT '',
      nik            varchar(16)    NOT NULL DEFAULT ''
   ) WITH ( OIDS = FALSE ) TABLESPACE pg_default;
)-");

const std::string t_base_roles( R"-(
   CREATE TABLE public.base_roles (
      id       serial PRIMARY KEY,
      name     varchar(50)    NOT NULL,
      alias    varchar(100)   NOT NULL DEFAULT '',
      enabled  boolean        NOT NULL DEFAULT true,
      created  timestamp      NOT NULL DEFAULT now(),
      updated  timestamp      NOT NULL DEFAULT now(),
      sysrole  boolean        NOT NULL DEFAULT false
   ) WITH (OIDS = FALSE) TABLESPACE pg_default;
)-");

const std::string t_base_sites( R"-(
   CREATE TABLE public.base_sites (
      id       serial PRIMARY KEY,
      code     varchar(7)    NOT NULL DEFAULT '',
      name     varchar(50)   NOT NULL DEFAULT '',
      address  varchar(512)  NOT NULL DEFAULT ''
   ) WITH (OIDS = FALSE) TABLESPACE pg_default;
)-");

const std::string t_base_user_role( R"-(
   CREATE TABLE public.base_user_role (
      id          serial   NOT NULL,
      user_id     integer  NOT NULL,
      role_id     integer  NOT NULL,
      is_primary  boolean  NOT NULL DEFAULT false,
      CONSTRAINT base_user_role_pkey PRIMARY KEY (user_id, role_id),
      CONSTRAINT base_user_role_role_id_fkey FOREIGN KEY (role_id)
         REFERENCES public.base_roles (id) MATCH SIMPLE ON UPDATE CASCADE ON DELETE CASCADE,
      CONSTRAINT base_user_role_user_id_fkey FOREIGN KEY (user_id)
         REFERENCES public.base_users (id) MATCH SIMPLE ON UPDATE CASCADE ON DELETE CASCADE
   ) WITH (OIDS = FALSE) TABLESPACE pg_default;
)-");

const std::string t_base_user_site( R"-(
   CREATE TABLE public.base_user_site (
      id          serial   NOT NULL,
      user_id     integer  NOT NULL,
      site_id     integer  NOT NULL,
      allow_login boolean  NOT NULL DEFAULT true,
      is_admin    boolean  NOT NULL DEFAULT false,
      CONSTRAINT base_user_site_pkey PRIMARY KEY (user_id, site_id),
      CONSTRAINT base_user_site_site_id_fkey FOREIGN KEY (site_id)
         REFERENCES public.base_sites (id) MATCH SIMPLE 
         ON UPDATE NO ACTION ON DELETE NO ACTION NOT VALID,
      CONSTRAINT base_user_site_user_id_fkey FOREIGN KEY (user_id)
         REFERENCES public.base_users (id) MATCH SIMPLE
         ON UPDATE CASCADE ON DELETE CASCADE
   ) WITH (OIDS = FALSE) TABLESPACE pg_default;
)-");

const std::string t_base_users_reset_password( R"-(
   CREATE TABLE public.base_users_reset_password (
      id           serial PRIMARY KEY,
      user_id      integer      NOT NULL,
      request_time timestamp    NOT NULL DEFAULT now(),
      expired_time bigint       NOT NULL,
      reset_code   varchar(255) NOT NULL,
      success      boolean      NOT NULL DEFAULT false,
      success_time timestamp    NOT NULL DEFAULT now()
   ) WITH (OIDS = FALSE) TABLESPACE pg_default;
)-");


const std::string t_base_acl( R"-(
   CREATE TABLE public.base_acl (
      id       serial         NOT NULL,
      ug_id    integer        NOT NULL,
      ug_type  varchar(1)     NOT NULL DEFAULT 'G',
      menu_id  integer        NOT NULL,
      a_all    boolean        NOT NULL DEFAULT true,
      a_add    boolean        NOT NULL DEFAULT true,
      a_delete boolean        NOT NULL DEFAULT true,
      a_update boolean        NOT NULL DEFAULT true,
      a_print  boolean        NOT NULL DEFAULT true,
      a_index  boolean        NOT NULL DEFAULT true,
      a_other  varchar(1024)  DEFAULT '',
      CONSTRAINT base_acl_pkey PRIMARY KEY (ug_id, ug_type, menu_id),
      CONSTRAINT base_acl_unique UNIQUE (id),
      CONSTRAINT "valid_ug_Type" CHECK (ug_type::text = 'U'::text OR ug_type::text = 'G'::text)
   ) WITH (OIDS = FALSE) TABLESPACE pg_default;
)-");

const std::string t_base_auth_log( R"-(
   CREATE TABLE public.base_auth_log (
      id          serial PRIMARY KEY,
      logon_time  timestamp    NOT NULL DEFAULT now(),
      usr_id      integer      NOT NULL,
      usr_name    varchar(255) NOT NULL,
      text_note   varchar(255) NOT NULL DEFAULT '',
      src_ip      varchar(100) NOT NULL DEFAULT '',
      src_host    varchar(255) NOT NULL DEFAULT '',
      src_mac     varchar(100) NOT NULL DEFAULT '',
      auth_type   varchar(30)  NOT NULL DEFAULT '',
      site_id     integer      NOT NULL,
      logged_out  boolean
   ) WITH (OIDS = FALSE) TABLESPACE pg_default;
)-");

const std::string t_base_class_code( R"-(
   CREATE TABLE public.base_class_code (
      id          serial PRIMARY KEY,
      item_name   varchar NOT NULL,
      item_class  varchar NOT NULL,
      item_id     integer NOT NULL,
      item_code   varchar NOT NULL
   ) WITH ( OIDS = FALSE ) TABLESPACE pg_default;
)-");

const std::string t_base_menu( R"-(
   CREATE TABLE public.base_menu (
      id       serial PRIMARY KEY,
      name     varchar(255) NOT NULL,
      group_id integer      NOT NULL,
      type_id  integer      NOT NULL,
      pageurl  varchar(255) NOT NULL,
      sidebar  boolean      NOT NULL DEFAULT false,
      methods  varchar(512) NOT NULL DEFAULT ''
   ) WITH ( OIDS = FALSE ) TABLESPACE pg_default;
)-");

const std::string t_company( R"-(
   CREATE TABLE public.company (
      id       serial PRIMARY KEY,
      name     varchar(255) NOT NULL DEFAULT '',
      address  varchar(512) NOT NULL DEFAULT '',
      phone    varchar(20)  NOT NULL DEFAULT '',
      email    varchar(255) NOT NULL DEFAULT '',
      website  varchar(255) NOT NULL DEFAULT ''
   ) WITH ( OIDS = FALSE ) TABLESPACE pg_default;
)-");

const std::string t_base_event_log( R"-(
   CREATE TABLE base_event_log (
      id          bigserial PRIMARY KEY,
      event_time  timestamp    NOT NULL DEFAULT now(),
      src_module  varchar(100) NOT NULL DEFAULT '',
      usr_id      integer      NOT NULL,
      usr_name    varchar(255) NOT NULL,
      text_note   varchar(512) NOT NULL DEFAULT ''
   ) WITH ( OIDS = FALSE ) TABLESPACE pg_default;
)-");

const std::string t_base_app_task( R"-(
   CREATE TABLE base_app_task (
      id             bigserial PRIMARY KEY,
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
   CREATE OR REPLACE VIEW public.v_base_acl AS
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
   CREATE OR REPLACE VIEW public.v_base_menu_group AS
   SELECT cc.item_id AS id, cc.item_name AS name, cc.item_code AS code
   FROM base_class_code cc
   WHERE cc.item_class = 'MENU_GROUP';
)-");

const std::string v_base_menu_type( R"-(
   CREATE OR REPLACE VIEW public.v_base_menu_type AS
   SELECT cc.item_id AS id, cc.item_name AS name, cc.item_code AS code
   FROM base_class_code cc
   WHERE cc.item_class = 'MENU_TYPE';
)-");

const std::string v_base_menu( R"-(
   CREATE OR REPLACE VIEW public.v_base_menu AS
   SELECT m.id, m.name,
      ( SELECT cc.item_code FROM base_class_code cc
            WHERE cc.item_id = m.type_id AND cc.item_class = 'MENU_TYPE') AS menu_type,
      ( SELECT cc.item_code FROM base_class_code cc
            WHERE cc.item_id = m.group_id AND cc.item_class = 'MENU_GROUP') AS menu_group,
      m.pageurl, m.sidebar, m.methods
   FROM base_menu m;
)-");

const std::string v_base_user_roles( R"-(
   CREATE OR REPLACE VIEW public.v_base_user_roles AS
   SELECT ur.id, ur.user_id, u.user_name, ur.role_id, r.name AS role_name
   FROM base_user_role ur
   JOIN base_users u ON u.id = ur.user_id
   JOIN base_roles r ON r.id = ur.role_id;
)-");

const std::string v_base_user_site( R"-(
   CREATE OR REPLACE VIEW public.v_base_user_site AS
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
      ('API_Server_Status', 1, 2, '/api/server_status', false, 'get'),
      ('API_Authenticate', 1, 2, '/api/authenticate', false, 'post'),
      ('API_Decrypt', 1, 2, '/api/decrypt', false, 'get'),
      ('API_Encrypt', 1, 2, '/api/encrypt', false, 'get'),
      ('API_Read_log_size_source', 1, 2, '/api/read_log/{size}/{source}', false, 'get'),
      ('API_Version', 1, 2, '/api/version', false, 'get'),
      ('API_Users', 1, 2, '/api/users', false, 'get'),
      ('API_Users_register', 1, 2, '/api/users/register', false, 'post'),
      ('API_Users_register_with_image', 1, 2, '/api/users/register_with_image', false, 'post'),
      ('API_Users_authenticate', 1, 2, '/api/users/authenticate', false, 'post'),
      ('API_Users_change_password', 1, 2, '/api/users/change_password', false, 'post'),
      ('API_Users_check_password', 1, 2, '/api/users/check_password', false, 'post'),
      ('API_Users_delete', 1, 2, '/api/users/delete', false, 'delete'),
      ('API_Users_forgot_password', 1, 2, '/api/users/forgot_password', false, 'post'),
      ('API_Users_id', 1, 2, '/api/users/{user_id}', false, 'get'),
      ('API_Users_exists_user_name', 1, 2, '/api/users/exists/{user_name}', false, 'get'),
      ('API_Users_reset_password', 1, 2, '/api/users/reset_password', false, 'post'),
      ('API_Users_update_profile', 1, 2, '/api/users/update_profile', false, 'put'),
      ('API_Users_update_profile_with_mage', 1, 2, '/api/users/update_profile_with_image', false, 'post'),
      ('API_Users_user_id_profile_image', 1, 2, '/api/users/{user_id}/profile_image', false, 'get'),
      ('API_Users_user_id_roles', 1, 2, '/api/users/{user_id}/roles', false, 'get'),
      ('API_Admin_ACL', 2, 2, '/api/admin/acl', false, 'post,delete'),
      ('API_Admin_Menus', 2, 2, '/api/admin/menus', false, 'post,delete'),
      ('API_Admin_Roles', 2, 2, '/api/admin/roles', false, 'post,delete'),
      ('API_Admin_Roles_addusers', 2, 2, '/api/admin/roles/addusers', false, 'post'),
      ('API_Admin_Roles_get_member', 2, 2, '/api/admin/roles/get_member', false, 'get'),
      ('API_Admin_Roles_get_non_member', 2, 2, '/api/admin/roles/get_non_member', false, 'get'),
      ('API_Admin_Roles_remove_user', 2, 2, '/api/admin/roles/remove_user', false, 'post'),
      ('API_Admin_Users', 2, 2, '/api/admin/users', false, 'post,delete'),
      ('API_Admin_Users_reset_password', 2, 2, '/api/admin/users/reset_password', false, 'post'),
      ('Admin', 2, 1, '/admin', true, 'get'),
      ('Admin_ACL', 2, 1, '/admin/acl', true, 'get'),
      ('Admin_Menus', 2, 1, '/admin/menus', true, 'get'),
      ('Admin_Roles', 2, 1, '/admin/roles', true, 'get'),
      ('Admin_Users', 2, 1, '/admin/users', true, 'get'),
      ('Home_About', 1, 1, '/about', true, 'all'),
      ('Home_Server_status', 1, 1, '/server_status', true, 'get'),
      ('Home_Dashboard', 1, 1, '/dashboard', true, 'get'),
      ('Home_Keep_alive', 1, 1, '/keep_alive', false, 'get'),
      ('Home_Login', 1, 1, '/login', true, 'get,post'),
      ('Home_Logout', 1, 1, '/logout', true, 'get'),
      ('Home_Page', 1, 1, '/', true, 'get'),
      ('Home_Password', 1, 1, '/password', true, 'get,post'),
      ('Home_Privacy', 1, 1, '/privacy', true, 'all'),
      ('Home_Register', 1, 1, '/register', true, 'get,post'),
      ('Home_Status_page', 1, 1, '/spage', true, 'get'),
      ('Home_User_profile', 1, 1, '/user_profile', false, 'get'),
      ('Home_Resource', 1, 1, '/resource/{resource_type}/{fileName}', false, 'get');
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
               decode('8420B2E991C17F6C102E4061F77B9DB914E9069C119B9CB159304C9BEAFFBE41', 'hex'), 
               decode('2632E5FBEE65C30D045C4A4FD9FDC971424A2B93A521D99E28DF855BD63687EA4293B3679E67E61D9E24C58475E4C6400AF01136C895D125767474734971D2A5', 'hex'),
               true, (now() + '1 year'::interval), '121211', '2000-01-01', '121212121211', 'M', 'JAKARTA', '1212121212121211' );
)-");


// First Admin user. Password is AdmUser9
const std::string insert_first_sys_user( R"-(
   INSERT INTO base_users (id, uuid, user_name, first_name, last_name, email, image, enabled, password_salt, password_hash,
                           allow_login, expired, unique_code, birth_date, phone, gender, address, nik )
      VALUES ( 3, '-DgT2vS8FYI1sENOkAlY', 'user1', 'User', 'User', 'user@mangapul.net', '', true,
               decode('247C5E834C87510639C78AD0999882EFD17EC8F7E04EC228AE1EF3AA9F450B91', 'hex'), 
               decode('154E408F4B4C7A1F6201561BD37877ABF7DB5FD2B7C3D3ABF1256C1321AE1335BD5723D990FFD06CE5473F51C49BC760DB4D6D694F77581F3ACAD44A37889CAD', 'hex'), 
               true, (now() + '1 year'::interval), '121213', '2000-01-01', '121212121213', 'M', 'JAKARTA', '1212121212121213' );
)-");

const std::string insert_base_user_role( R"-(
   INSERT INTO base_user_role (id, user_id, role_id, is_primary) 
   VALUES (1, 1, 1, true),   (3, 3, 2, true);
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

} // namespace pgsql
} // namespace dbm
} // namespace tbs

#endif //defined(TOBASA_SQL_USE_PGSQL)