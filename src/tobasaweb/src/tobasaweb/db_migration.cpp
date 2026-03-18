#pragma once

#include <vector>
#include <functional>
#include <string>

#include "tobasaweb/db_migration.h"

namespace tbs {
namespace dbm {

template<typename Driver>
void runDbMigrationImpl(const sql::conf::Database* opt, const std::string& securitySalt, MigrationJob& job)
{
   using DbConn   = sql::SqlConnection<Driver>;
   using DbQuery  = sql::SqlQuery<Driver>;
   using DbResult = sql::SqlResult<Driver>;

   try
   {
      DbConn conn;
      auto connStr = util::getConnectionString(*opt, securitySalt);

      if (conn.connect(connStr))
      {
         if (opt->dbDriver == sql::BackendType::adodb || opt->dbDriver == sql::BackendType::odbc) 
         {
            conn.executeVoid(
               " CREATE TABLE IF NOT EXISTS schema_migrations ( "
               " version VARCHAR(50) NOT NULL, "
               " module_name VARCHAR(50) NOT NULL, "
               " note VARCHAR(200), " 
               " CONSTRAINT PK_schema_migrations PRIMARY KEY (version, module_name) ) "
            );
         }
         else
         {
            conn.executeVoid(
               " CREATE TABLE IF NOT EXISTS schema_migrations ( "
               " version VARCHAR(50) NOT NULL, "
               " module_name VARCHAR(50) NOT NULL, "
               " note VARCHAR(200), " 
               " PRIMARY KEY (version, module_name)  ) "
            );
         }

         job.run(conn);
      }
      else
         Logger::logE("Could not open db connection, skipping db migration check");
   }
   catch (const std::exception& ex) 
   {
      Logger::logE("Error during db migration check: {}", ex.what());
   }
}

void runDbMigration(const sql::conf::ConnectorOption* option, MigrationJob& job)
{
   sql::conf::Database opt = option->production;
   if (option->environment == "development")
      opt = option->development;

 // -------------------------------------------------------
 #if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
   if (opt.dbDriver == sql::BackendType::adodb)
   {
      if ( FAILED(::CoInitializeEx(NULL, COINIT_MULTITHREADED)) )
         Logger::logD("[sql] Initializing ADODB COM library has failed");
      else
         Logger::logD("[sql] Initializing ADODB COM library");
   }
 #endif
 // -------------------------------------------------------

 #if defined(TOBASA_SQL_USE_SQLITE)
   if (opt.dbDriver == sql::BackendType::sqlite)
      runDbMigrationImpl<sql::SqliteDriver>(&opt, option->securitySalt, job);
 #endif

 #if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
   if (opt.dbDriver == sql::BackendType::adodb)
      runDbMigrationImpl<sql::AdodbDriver>(&opt, option->securitySalt, job);
 #endif

 #if defined(TOBASA_SQL_USE_ODBC)
   if (opt.dbDriver == sql::BackendType::odbc)
      runDbMigrationImpl<sql::OdbcDriver>(&opt, option->securitySalt, job);
 #endif

 #if defined(TOBASA_SQL_USE_PGSQL)
   if (opt.dbDriver == sql::BackendType::pgsql)
      runDbMigrationImpl<sql::PgsqlDriver>(&opt, option->securitySalt, job);
 #endif

 #if defined(TOBASA_SQL_USE_MYSQL)
   if (opt.dbDriver == sql::BackendType::mysql)
      runDbMigrationImpl<sql::MysqlDriver>(&opt, option->securitySalt, job);
 #endif

 // -------------------------------------------------------
 #if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
   if (opt.dbDriver == sql::BackendType::adodb)
   {
      Logger::logD("[sql] Uninitializing ADODB COM library");
      ::CoUninitialize();
   }
 #endif
 // -------------------------------------------------------
}


}} // namespace tbs::dbm