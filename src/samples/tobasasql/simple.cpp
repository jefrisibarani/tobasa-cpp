#include <iostream>
#include <tobasa/datetime.h>
#include <tobasa/logger.h>
#include <tobasasql/sql_driver.h>
#include <tobasasql/sql_connection.h>
#include <tobasasql/sql_query.h>
#include <tobasasql/sql_table.h>


int main()
{
   using namespace tbs;

   // We need this for Tobasa DateTime objects or SQL date/time conversion:
   if (! tbs::DateTime::initTimezoneData())
      return 1;

   std::cout << "TOBASA SQL Drivers\n";
   
   // We have to set log target for tbs::Logger
   tbs::Logger::setTarget(new tbs::log::CoutLogSink()) ;

   std::string connString;

#if defined(TOBASA_SQL_USE_SQLITE)   
   try
   {
      connString = "Database=./tbs_coba.db3;OpenCreate=True;OpenMemory=False;Password=AdmBaru98;";
      sql::SqlConnection<sql::SqliteDriver> conn;
      if (conn.connect(connString))
      {
            auto currentTime = conn.executeScalar("SELECT CURRENT_TIMESTAMP");
            std::cout << "SQLITE Driver\n";
            std::cout << "Current time    : " << currentTime << "\n";
            std::cout << "Backend Version : " << conn.versionString() << "\n";
            std::cout << "\n";
      }
   }
   catch(const std::exception& ex)
   {
      std::cerr << "Exception : " << ex.what();
   }
#endif

#if defined(TOBASA_SQL_USE_PGSQL)
   try
   {
      connString = "dbname=postgres user=postgres password=@PGuser2004 hostaddr=10.62.22.2 port=5462";
      sql::SqlConnection<sql::PgsqlDriver> conn;
      if (conn.connect(connString))
      {
         auto currentTime = conn.executeScalar("SELECT CURRENT_TIMESTAMP");
         std::cout << "PGSQL Driver\n";
         std::cout << "Current time    : " << currentTime << "\n";
         std::cout << "Backend Version: " << conn.versionString() << "\n";
         std::cout << "\n";
         conn.disconnect();
      }
   }
   catch(const std::exception& ex)
   {
      std::cerr << "Exception : " << ex.what();
   }
#endif

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
   try
   {
      std::cout << "\n";
      if ( FAILED(::CoInitializeEx(NULL, COINIT_MULTITHREADED)) )
         std::cerr << "Initializing ADODB COM library has failed" << "\n";
      else
         std::cout << "Initializing ADODB COM library" << "\n";

      connString = "Provider=SQLNCLI11;Server=10.62.22.2;Database=master;Uid=tbs_user;Pwd=AdmBaru98;DataTypeCompatibility=80;";
      sql::SqlConnection<sql::AdodbDriver> conn;
      if (conn.connect(connString))
      {
         auto currentTime = conn.executeScalar("SELECT GETDATE()");
         std::cout << "ADODB Driver\n";
         std::cout << "Current time    : " << currentTime << "\n";
         std::cout << "Backend Version : " << conn.versionString() << "\n";
         conn.disconnect();
      } 
      std::cout << "Uninitializing ADODB COM library\n";
      std::cout << "\n";
      ::CoUninitialize();
   }
   catch(const std::exception& ex)
   {
      std::cerr << "Exception : " << ex.what();
   }      
#endif

#if defined(TOBASA_SQL_USE_ODBC)
   try
   {
      connString = "Driver={ODBC Driver 17 for SQL Server};Server=10.62.22.2;Database=master;Uid=tbs_user;Pwd=AdmBaru98;TrustServerCertificate=Yes;";
      sql::SqlConnection<sql::OdbcDriver> conn;
      if (conn.connect(connString))
      {
         auto currentTime = conn.executeScalar("SELECT GETDATE()");
         std::cout << "ODBC Driver\n";
         std::cout << "Current time    : " << currentTime << "\n";
         std::cout << "Backend Version : " << conn.versionString() << "\n";
         std::cout << "\n";
         conn.disconnect();
      }
   }
   catch(const std::exception& ex)
   {
      std::cerr << "Exception : " << ex.what();
   }      
#endif

#if defined(TOBASA_SQL_USE_MYSQL)
   try
   {
      connString = "Database=tbs_user;User=tbs_user;Password=AdmBaru98;Server=10.62.22.2;Port=3306";
      sql::SqlConnection<sql::MysqlDriver> conn;
      if (conn.connect(connString))
      {
         conn.setLogSqlQuery(true);
         conn.setLogSqlQueryInternal(true);
         conn.setLogExecuteStatus(true);

         auto currentTime = conn.executeScalar("SELECT NOW()");
         std::cout << "MYSQL Driver\n";
         std::cout << "Current time    : " << currentTime << "\n";
         std::cout << "Backend Version : " << conn.versionString() << "\n";

         std::cout << "\n";
         conn.disconnect();
      }
   }
   catch(const std::exception& ex)
   {
      std::cerr << "Exception : " << ex.what();
   }
#endif

   return 0;
}