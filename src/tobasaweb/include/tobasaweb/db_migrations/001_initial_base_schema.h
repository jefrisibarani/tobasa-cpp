#pragma once

#include <tobasasql/sql_connection.h>

#include "schema_sqlite.h"
#include "schema_mysql.h"
#include "schema_mssql.h"
#include "schema_pgsql.h"

namespace tbs { 
namespace dbm {

class InitialBaseSchema
{
public:

   static std::string version() { return "001"; }
   static std::string moduleName() { return "BASE"; }
   static std::string note() { return "Initial BASE schema"; }

   template<typename Driver>
   static void up(sql::SqlConnection<Driver>& conn)
   {
      using DbQuery  = sql::SqlQuery<Driver>;

      std::vector<std::string> queries;

 #if (defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)) || defined(TOBASA_SQL_USE_ODBC)
      if (conn.backendType() == sql::BackendType::adodb || conn.backendType() == sql::BackendType::odbc)
         queries = dbm::mssql::getQueries();
 #endif

 #if defined(TOBASA_SQL_USE_SQLITE)
      if (conn.backendType() == sql::BackendType::sqlite)
         queries = dbm::sqlite::getQueries();
 #endif

 #if defined(TOBASA_SQL_USE_PGSQL)
      if (conn.backendType() == sql::BackendType::pgsql)
         queries = dbm::pgsql::getQueries();
 #endif

 #if defined(TOBASA_SQL_USE_MYSQL)
      if (conn.backendType() == sql::BackendType::mysql)
         queries = dbm::mysql::getQueries();
 #endif

      for (auto cmd: queries)
      {
         conn.executeVoid(cmd);
      }

      // ACL for Users (role id 2)
      {
         std::string sql = "SELECT id, name FROM base_menu WHERE group_id=1";
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