#pragma once

#include <tobasasql/sql_connection.h>

namespace tbs { 
namespace dbm {

class InitialBaseModuleTest
{
public:

   static std::string version() { return "002"; }
   static std::string moduleName() { return "BASE"; }
   static std::string note() { return "Initial Base Module TEST schema"; }

   template<typename Driver>
   static void up(sql::SqlConnection<Driver>& conn)
   {
      using DbQuery  = sql::SqlQuery<Driver>;

      // Test menu group class code (id 8)
      conn.executeVoid("INSERT INTO base_class_code (item_name, item_class, item_id, item_code) VALUES ('TEST', 'MENU_GROUP', 8, 'RES_4') ");

      // TEST menus
      {
         std::string insert_base_menu_test( R"-(
               INSERT INTO base_menu (name, group_id, type_id, pageurl, sidebar, methods) VALUES
                  ('Home_Test_Websocket', 8, 1, '/test_websocket',  1, 'get'),
                  ('Home_Websocket_a',    8, 1, '/websocket_ep',    0, 'get'),
                  ('Home_Websocket_b',    8, 1, '/websocket_ep_1',  0, 'get');
            )-");

         if (conn.backendType() == sql::BackendType::pgsql)
         {
            insert_base_menu_test = ( R"-(
               INSERT INTO base_menu (name, group_id, type_id, pageurl, sidebar, methods) VALUES
                  ('Home_Test_Websocket', 8, 1, '/test_websocket',  true,  'get'),
                  ('Home_Websocket_a',    8, 1, '/websocket_ep',    false, 'get'),
                  ('Home_Websocket_b',    8, 1, '/websocket_ep_1',  false, 'get');
            )-");
         }
         
         conn.executeVoid(insert_base_menu_test);
      }

      // Setup ACL for Users (role id 2) in Test Menu group
      {
         std::string sql = "SELECT id, name FROM base_menu WHERE group_id=8";
         DbQuery query(conn,sql);
         auto result = query.executeResult();
         if (result != nullptr && result->isValid() && result->totalRows() > 0)
         {
            for (int i = 0; i < result->totalRows(); i++)
            {
               result->locate(i);
               auto menuId = result->getStringValue("id");
               sql = tbsfmt::format("INSERT INTO base_acl (ug_id, ug_type, menu_id) VALUES (2, 'G', {} );", menuId);
               conn.executeVoid(sql);
            }
         }
      }

   }
};


} } // namespace tbs::dbm