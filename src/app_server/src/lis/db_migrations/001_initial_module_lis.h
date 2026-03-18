#pragma once

#include <tobasasql/sql_connection.h>

#include "schema_sqlite.h"
#include "schema_mysql.h"
#include "schema_mssql.h"
#include "schema_pgsql.h"

namespace tbs { 
namespace dbm {

class InitialModuleLIS
{
public:

   static std::string version() { return "001"; }
   static std::string moduleName() { return "LIS"; }
   static std::string note() { return "Initial Module LIS schema"; }

   template<typename Driver>
   static void up(sql::SqlConnection<Driver>& conn)
   {
      using DbQuery  = sql::SqlQuery<Driver>;

      std::vector<std::string> queries;

 #if (defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)) || defined(TOBASA_SQL_USE_ODBC)
      if (conn.backendType() == sql::BackendType::adodb || conn.backendType() == sql::BackendType::odbc)
         queries = dbm::mssql::getLisQueries();
 #endif

 #if defined(TOBASA_SQL_USE_SQLITE)
      if (conn.backendType() == sql::BackendType::sqlite)
         queries = dbm::sqlite::getLisQueries();
 #endif

 #if defined(TOBASA_SQL_USE_PGSQL)
      if (conn.backendType() == sql::BackendType::pgsql)
         queries = dbm::pgsql::getLisQueries();
 #endif

 #if defined(TOBASA_SQL_USE_MYSQL) 
      if (conn.backendType() == sql::BackendType::mysql)
         queries = dbm::mysql::getLisQueries();
 #endif

      for (auto cmd: queries)
      {
         conn.executeVoid(cmd);
      }

   }
};


} } // namespace tbs::dbm