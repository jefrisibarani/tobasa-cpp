#pragma once

#include <vector>
#include <functional>
#include <string>
#include <tobasasql/sql_connection.h>
#include <tobasasql/sql_query.h>
#include <tobasasql/sql_driver.h>
#include <tobasasql/settings.h>

namespace tbs {
namespace dbm {

template<typename Driver, typename Migration>
void doRunMigration(sql::SqlConnection<Driver>& conn)
{
   std::string check =
      tbsfmt::format("SELECT COUNT(*) FROM schema_migrations WHERE version='{}' AND module_name='{}' ",
         Migration::version(), Migration::moduleName());

   if (conn.executeScalar(check) == "0")
   {
      Logger::logI("Running migration {} :{} :{}", Migration::version(), Migration::moduleName(), Migration::note() );

      conn.executeVoid("BEGIN");

      try
      {
         Migration::up(conn);

         conn.executeVoid(
            tbsfmt::format("INSERT INTO schema_migrations(version, module_name, note) VALUES('{}','{}','{}')",
               Migration::version(), Migration::moduleName(), Migration::note()));

         conn.executeVoid("COMMIT");
      }
      catch (...)
      {
         conn.executeVoid("ROLLBACK");
         throw;
      }
   }
}


class MigrationJob
{
   friend class Webapp;
public:

   template<typename Migration>
   MigrationJob& add()
   {
      _runners.push_back(&runMigration<Migration>);
      return *this;
   }

   template<typename Driver>
   void run(sql::SqlConnection<Driver>& conn)
   {
      for (auto fn : _runners)
         fn(&conn, typeid(Driver));
   }

private:

   using RunnerFn = void(*)(void*, const std::type_info&);

   std::vector<RunnerFn> _runners;

   template<typename Migration>
   static void runMigration(void* c, const std::type_info& t)
   {
 #if defined(TOBASA_SQL_USE_SQLITE)
      if (t == typeid(sql::SqliteDriver))
      {
         auto& conn = *static_cast<sql::SqlConnection<sql::SqliteDriver>*>(c);
         doRunMigration<sql::SqliteDriver, Migration>(conn);
      }
 #endif

 #if defined(TOBASA_SQL_USE_MYSQL)
      if (t == typeid(sql::MysqlDriver))
      {
         auto& conn = *static_cast<sql::SqlConnection<sql::MysqlDriver>*>(c);
         doRunMigration<sql::MysqlDriver, Migration>(conn);
      }
 #endif

 #if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER) 
      if (t == typeid(sql::AdodbDriver))
      {
         auto& conn = *static_cast<sql::SqlConnection<sql::AdodbDriver>*>(c);
         doRunMigration<sql::AdodbDriver, Migration>(conn);
      }
 #endif

 #if defined(TOBASA_SQL_USE_PGSQL)
      if (t == typeid(sql::PgsqlDriver))
      {
         auto& conn = *static_cast<sql::SqlConnection<sql::PgsqlDriver>*>(c);
         doRunMigration<sql::PgsqlDriver, Migration>(conn);
      }
 #endif

 #if defined(TOBASA_SQL_USE_ODBC)
      if (t == typeid(sql::OdbcDriver))
      {
         auto& conn = *static_cast<sql::SqlConnection<sql::OdbcDriver>*>(c);
         doRunMigration<sql::OdbcDriver, Migration>(conn);
      }
 #endif
   }

};


void runDbMigration(const sql::conf::ConnectorOption* option, MigrationJob& job);

}} // namespace tbs::dbm