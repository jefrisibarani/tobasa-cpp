#pragma once

#include <tobasasql/sql_connection.h>

namespace tbs { 
namespace dbm {

class InitialBaseModuleLIS
{
public:

   static std::string version() { return "003"; }
   static std::string moduleName() { return "BASE"; }
   static std::string note() { return "Initial Base Module LIS schema"; }

   template<typename Driver>
   static void up(sql::SqlConnection<Driver>& conn)
   {
      using DbQuery  = sql::SqlQuery<Driver>;

      // LIS menu group class code (id 6)
      conn.executeVoid("INSERT INTO base_class_code (item_name, item_class, item_id, item_code) VALUES ('LIS', 'MENU_GROUP', 6, 'RES_2') ");

      // LIS menus
      {
         std::string insert_base_menu_lis( R"-(
               INSERT INTO base_menu (name, group_id, type_id, pageurl, sidebar, methods) VALUES
                  ('API_LIS_engine_state',            6, 2, '/api/lis/server_status',                    0, 'get'),
                  ('API_LIS_parse_and_send_message',  6, 2, '/api/lis/parse_and_send_message',           0, 'post'),
                  ('API_LIS_send_message',            6, 2, '/api/lis/send_lis_message',                 0, 'post'),
                  ('API_LIS_send_message_HL7',        6, 2, '/api/lis/send_hl7_message',                 0, 'post'),
                  ('API_LIS_start_engine',            6, 2, '/api/lis/start_engine',                     0, 'post'),
                  ('API_LIS_stop_engine',             6, 2, '/api/lis/stop_engine',                      0, 'post'),
                  ('API_LIS_lis2a_result_list',       6, 2, '/api/lis/lis2a_result_list/{header_id}',    0, 'get'),
                  ('API_LIS_hl7_obxlist',             6, 2, '/api/lis/hl7_obxlist/{obrid}/{patientid}',  0, 'get'),
                  ('LIS_Server_status',               6, 1, '/lis/server_status',                        1, 'get'),
                  ('LIS_TestDev_LIS1A',               6, 1, '/lis/testdev_lis1a',                        1, 'get'),
                  ('LIS_TestDev_HL7',                 6, 1, '/lis/testdev_hl7',                          1, 'get');
            )-");

         if (conn.backendType() == sql::BackendType::pgsql)
         {
            insert_base_menu_lis = ( R"-(
               INSERT INTO base_menu (name, group_id, type_id, pageurl, sidebar, methods) VALUES
                  ('API_LIS_engine_state',            6, 2, '/api/lis/server_status',                    false, 'get'),
                  ('API_LIS_parse_and_send_message',  6, 2, '/api/lis/parse_and_send_message',           false, 'post'),
                  ('API_LIS_send_message',            6, 2, '/api/lis/send_lis_message',                 false, 'post'),
                  ('API_LIS_send_message_HL7',        6, 2, '/api/lis/send_hl7_message',                 false, 'post'),
                  ('API_LIS_start_engine',            6, 2, '/api/lis/start_engine',                     false, 'post'),
                  ('API_LIS_stop_engine',             6, 2, '/api/lis/stop_engine',                      false, 'post'),
                  ('API_LIS_lis2a_result_list',       6, 2, '/api/lis/lis2a_result_list/{header_id}',    false, 'get'),
                  ('API_LIS_hl7_obxlist',             6, 2, '/api/lis/hl7_obxlist/{obrid}/{patientid}',  false, 'get'),
                  ('LIS_Server_status',               6, 1, '/lis/server_status',                        true, 'get'),
                  ('LIS_TestDev_LIS1A',               6, 1, '/lis/testdev_lis1a',                        true, 'get'),
                  ('LIS_TestDev_HL7',                 6, 1, '/lis/testdev_hl7',                          true, 'get');
            )-");
         }
         conn.executeVoid(insert_base_menu_lis);
      }


      // Role LIS users (id 5)
      {
         if (conn.backendType() == sql::BackendType::adodb || conn.backendType() == sql::BackendType::odbc)
         {
            std::string cmd( R"-(
                  SET IDENTITY_INSERT base_roles ON;
                  INSERT INTO base_roles (id, name, alias, enabled, sysrole) VALUES (5, 'role_lis_user', 'LIS User', 1, 1);
                  SET IDENTITY_INSERT base_roles OFF;
               )-");

            conn.executeVoid(cmd);
         }
         else if (conn.backendType() == sql::BackendType::mysql || conn.backendType() == sql::BackendType::sqlite)
         {
            conn.executeVoid("INSERT INTO base_roles (id, name, alias, enabled, sysrole) VALUES (5, 'role_lis_user', 'LIS User', 1, 1) ");
         }
         else if (conn.backendType() == sql::BackendType::pgsql )
         {
            conn.executeVoid("INSERT INTO base_roles (id, name, alias, enabled, sysrole) VALUES (5, 'role_lis_user', 'LIS User', true, true) ");
         }
      }

      // ACL menu group LIS (id 6) , role id 5
      {
         std::string sql = "SELECT id, name FROM base_menu WHERE group_id=6";
         DbQuery query(conn,sql);
         auto result = query.executeResult();
         if (result != nullptr && result->isValid() && result->totalRows() > 0)
         {
            for (int i = 0; i < result->totalRows(); i++)
            {
               result->locate(i);

               auto menuId = result->getStringValue("id");
               sql = tbsfmt::format("INSERT INTO base_acl (ug_id, ug_type, menu_id) VALUES (5, 'G', {} );", menuId);
               conn.executeVoid(sql);
            }
         }
      }

   }
};


} } // namespace tbs::dbm