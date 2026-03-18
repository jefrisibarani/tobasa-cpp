#include <tobasa/logger.h>
#include <tobasa/config.h>
#include <tobasa/json.h>
#include <tobasaweb/settings_webapp.h>
#include <tobasasql/exception.h>
#include <tobasaweb/json_result.h>
#include "test_sqldriver.h"
#include "test_controller.h"

namespace tbs {
namespace test {

using namespace http;

TestController::~TestController()
{
   auto appOption = Config::getOption<web::conf::Webapp>("webapp");
   auto dbOption = appOption.dbConnection.production;
   if (appOption.dbConnection.environment == "development") {
      dbOption = appOption.dbConnection.development;
   }

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
   // Uninitialize COM
   // only do this if Webapp main database driver is not adodb
   if (dbOption.dbDriver != sql::BackendType::adodb)
   {
      Logger::logI("[test] TestController: Uninitializing COM library");
      ::CoUninitialize();
   }
#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
}


void TestController::onInit()
{
   auto appOption = Config::getOption<web::conf::Webapp>("webapp");
   auto dbOption  = appOption.dbConnection.production;
   if (appOption.dbConnection.environment == "development") {
      dbOption = appOption.dbConnection.development;
   }

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
   // Initialize COM
   // only do this if Webapp main database driver is not adodb
   if (dbOption.dbDriver != sql::BackendType::adodb)
   {
      if ( FAILED(::CoInitializeEx(NULL, COINIT_MULTITHREADED)) )
         Logger::logI("[test] TestController: Initializing COM library has failed");
      else
         Logger::logI("[test] TestController: COM library initialized");
   }
#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

}


void TestController::bindHandler()
{
   auto self(this);
   auto by = [&]( auto method ) {
      using namespace std::placeholders;
      return std::bind( method, self, _1);
   };

   //! Handle POST request to /test/crypto
   router()->httpPost("/test/crypto",     by(&TestController::onCrypto) );
   //! Handle POST request to /test/datetime
   router()->httpPost("/test/datetime",   by(&TestController::onDateTime) );

   //! Handle POST request to /test/pgsql
   router()->httpPost("/test/pgsql",      by(&TestController::onSql) );
   //! Handle POST request to /test/adosql
   router()->httpPost("/test/adosql",     by(&TestController::onSql) );
   //! Handle POST request to /test/sqlite
   router()->httpPost("/test/sqlite",     by(&TestController::onSql) );
   //! Handle POST request to /test/odbc_mssql
   router()->httpPost("/test/odbc_mssql", by(&TestController::onSql) );
   //! Handle POST request to /test/odbc_mysql
   router()->httpPost("/test/odbc_mysql", by(&TestController::onSql) );
   //! Handle POST request to /test/mysql
   router()->httpPost("/test/mysql",      by(&TestController::onSql) );


   router()->httpPost("/test/upload",     by(&TestController::onUpload) );
}


//! Handle POST request to /test/pgsql
//! Handle POST request to /test/adosql
//! Handle POST request to /test/sqlite
//! Handle POST request to /test/odbc_mssql
//! Handle POST request to /test/odbc_mysql
//! Handle POST request to /test/mysql
http::ResultPtr TestController::onSql(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto request = httpCtx->request();

   if (!util::startsWith(httpCtx->request()->contentType(), "application/json"))
      return jsonResult(StatusCode::BAD_REQUEST);

   try
   {
      // Main application sql connection object
      auto& connPtrVariant = this->dbServiceFactory()->sqlConnPtrVariant();
      std::string versionStringMain =
         std::visit([&](auto& conn)
            { return conn->versionString(); }, connPtrVariant );

      if( request->path() == "/test/pgsql")
      {
#if defined(TOBASA_SQL_USE_PGSQL)         
         TestSqlDriver<sql::PgsqlDriver> testSql(request->content());
         testSql.jsonResult()["versionStringMain"] = versionStringMain;
         testSql.start();
         return jsonResult(testSql.jsonResult());
#else
         return jsonResult(StatusCode::NOT_IMPLEMENTED);
#endif 

      }

      if( request->path() == "/test/adosql")
      {
#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
         TestSqlDriver<sql::AdodbDriver> testSql(request->content());
         testSql.jsonResult()["versionStringMain"] = versionStringMain;
         testSql.start();
         return jsonResult(testSql.jsonResult());

#else
         return jsonResult(StatusCode::NOT_IMPLEMENTED);
#endif 
      }

      if( request->path() == "/test/sqlite")
      {
#if defined(TOBASA_SQL_USE_SQLITE)         
         TestSqlDriver<sql::SqliteDriver> testSql(request->content());
         testSql.jsonResult()["versionStringMain"] = versionStringMain;
         testSql.start();
         return jsonResult(testSql.jsonResult());
#else
         return jsonResult(StatusCode::NOT_IMPLEMENTED);
#endif          
      }

      if( request->path() == "/test/odbc_mssql")
      {
#if defined(TOBASA_SQL_USE_ODBC)         
         TestSqlDriver<sql::OdbcDriver> testSql(request->content());
         testSql.jsonResult()["versionStringMain"] = versionStringMain;
         testSql.start();
         return jsonResult(testSql.jsonResult());
#else
         return jsonResult(StatusCode::NOT_IMPLEMENTED);
#endif         
      }

      if( request->path() == "/test/odbc_mysql")
      {
#if defined(TOBASA_SQL_USE_ODBC) 
         TestSqlDriver<sql::OdbcDriver> testSql(request->content());
         testSql.jsonResult()["versionStringMain"] = versionStringMain;
         testSql.start();
         return jsonResult(testSql.jsonResult());
#else
         return jsonResult(StatusCode::NOT_IMPLEMENTED);
#endif         
      }

      if( request->path() == "/test/mysql")
      {
#if defined(TOBASA_SQL_USE_MYSQL)          
         TestSqlDriver<sql::MysqlDriver> testSql(request->content());
         testSql.jsonResult()["versionStringMain"] = versionStringMain;
         testSql.start();
         return jsonResult(testSql.jsonResult());
#else
         return jsonResult(StatusCode::NOT_IMPLEMENTED);
#endif            
      }

      return jsonResult(StatusCode::NOT_FOUND);

   }
   catch (const SqlException &ex)
   {
      Logger::logE("[webapp] [conn:{}] {}", httpCtx->connId(), ex.what());
      return jsonResult(StatusCode::INTERNAL_SERVER_ERROR, "SQL server error");
   }
   catch (const tbs::AppException &ex)
   {
      Logger::logE("[webapp] [conn:{}] {}", httpCtx->connId(), ex.what());
      return jsonResult(StatusCode::INTERNAL_SERVER_ERROR, ex.what());
   }
   catch (const Json::exception &ex)
   {
      Logger::logE("[webapp] [conn:{}] {}", httpCtx->connId(), ex.what());
      return jsonResult(StatusCode::BAD_REQUEST, "JSON data error: " + cleanJsonException(ex));
   }
   catch (const std::exception &ex)
   {
      Logger::logE("[webapp] [conn:{}] {}", httpCtx->connId(), ex.what());
      return jsonResult(StatusCode::INTERNAL_SERVER_ERROR);
   }
}

} // namespace test
} // namespace tbs
