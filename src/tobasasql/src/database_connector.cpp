#include <tobasa/crypt.h>
#include "tobasasql/util.h"
#include "tobasasql/database_connector.h"

namespace tbs {
namespace sql {

DatabaseConnector::DatabaseConnector(const sql::conf::ConnectorOption& dbConnOption, const std::string& name)
   : DatabaseConnectorBase(name)
   , _dbConnOption(dbConnOption)
{
   _dbOption = _dbConnOption.production;
   if (_dbConnOption.environment == "development") {
      _dbOption = _dbConnOption.development;
   }

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
   // Initialize COM
   if (_dbOption.dbDriver == sql::BackendType::adodb)
   {
      if ( FAILED(::CoInitializeEx(NULL, COINIT_MULTITHREADED)) )
         Logger::logD("[sql] Initializing ADODB COM library has failed");
      else
         Logger::logD("[sql] Initializing ADODB COM library");
   }
#endif
}

DatabaseConnector::~DatabaseConnector()
{
   disconnect();

#if defined(TOBASA_SQL_USE_PGSQL)
   _pPgsqlConn = nullptr;
#endif

#if defined(TOBASA_SQL_USE_SQLITE)
   _pSqliteConn = nullptr;
#endif

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
   _pAdodbConn = nullptr;
#endif

#if defined(TOBASA_SQL_USE_ODBC)
   _pOdbcConn = nullptr;
#endif

#if defined(TOBASA_SQL_USE_MYSQL)
   _pMysqlConn = nullptr;
#endif

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
   // Uninitialize COM
   if (_dbOption.dbDriver == sql::BackendType::adodb)
   {
      Logger::logD("[sql] Uninitializing ADODB COM library");
      ::CoUninitialize();
   }
#endif

   Logger::logD("[sql] DatabaseConnector destroyed");
}

bool DatabaseConnector::connect()
{
   auto connString = util::getConnectionString(_dbOption, _dbConnOption.securitySalt);

   return std::visit(
      [&](auto& conn)
      {
         if (conn->backendType() == _dbOption.dbDriver)
         {
            conn->setLogSqlQuery(_dbConnOption.logSqlQuery);
            conn->setLogSqlQueryInternal(_dbConnOption.logInternalSqlQuery);
            conn->setLogExecuteStatus(_dbConnOption.logSqlQuery);
            bool ok = conn->connect(connString);
            if (ok)
            {
#if defined(TOBASA_SQL_USE_SQLITE)
               if (conn->backendType() == BackendType::sqlite)
               {
                  conn->executeVoid("PRAGMA foreign_keys = ON;");
                  // Database locked can be avoided by setting busy_timeout
                  // or set journal_mode = WAL
                  conn->executeVoid("PRAGMA busy_timeout = 6000;");
                  conn->executeVoid("PRAGMA journal_mode = WAL;");
               }
#endif
            }
            return ok;
         }
         else
            return false;
      }, _sqlConnPtrVariant);
}

bool DatabaseConnector::disconnect()
{
   return std::visit(
      [&](auto& conn)
      {
         if (conn->backendType() == _dbOption.dbDriver) {
            return conn->disconnect();
         }
         else
            return true;
      }, _sqlConnPtrVariant);
}

bool DatabaseConnector::connected()
{
   return std::visit(
      [&](auto& conn)
      {
         if (conn->backendType() == _dbOption.dbDriver)
         {
            bool isOk = conn->status() == sql::ConnectionStatus::ok;
            return isOk;
         }
         else
            return false;
      }, _sqlConnPtrVariant);
}

void DatabaseConnector::initSqlDriver()
{
   using namespace sql;
   switch (_dbOption.dbDriver)
   {

#if defined(TOBASA_SQL_USE_PGSQL)
   case BackendType::pgsql:
      {
         _sqlDriverVariant  = _pgsqlDriver;
         _pPgsqlConn        = std::make_shared<SqlConnection<PgsqlDriver>>();
         _sqlConnPtrVariant = _pPgsqlConn;
         break;
      }
#endif

#if defined(TOBASA_SQL_USE_SQLITE)
   case BackendType::sqlite:
      {
         _sqlDriverVariant  = _sqliteDriver;
         _pSqliteConn       = std::make_shared<SqlConnection<SqliteDriver>>();
         _sqlConnPtrVariant = _pSqliteConn;
         break;
      }
#endif

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
   case BackendType::adodb:
      {
         _sqlDriverVariant  = _adodbDriver;
         _pAdodbConn        = std::make_shared<SqlConnection<AdodbDriver>>();
         _sqlConnPtrVariant = _pAdodbConn;
         break;
      }
#endif

#if defined(TOBASA_SQL_USE_ODBC)
   case BackendType::odbc:
      {
         _sqlDriverVariant  = _odbcDriver;
         _pOdbcConn         = std::make_shared<SqlConnection<OdbcDriver>>();
         _sqlConnPtrVariant = _pOdbcConn;
         break;
      }
#endif

#if defined(TOBASA_SQL_USE_MYSQL)
   case BackendType::mysql:
      {
         _sqlDriverVariant  = _mysqlDriver;
         _pMysqlConn        = std::make_shared<SqlConnection<MysqlDriver>>();
         _sqlConnPtrVariant = _pMysqlConn;
         break;
      }
#endif

   default:
      throw AppException("Unknown SQL driver type");
      break;
   }
}

bool DatabaseConnector::testConnection()
{
   return std::visit(
      [&](auto& conn)
      {
         if (conn->backendType() == _dbOption.dbDriver) {
            return conn->executeVoid("SELECT 1;");
         }
         else
            return false;
      }, _sqlConnPtrVariant);
}

bool DatabaseConnector::beginTransaction()
{
   return std::visit(
      [&](auto& conn)
      {
         try
         {
            if (conn->backendType() == _dbOption.dbDriver) 
            {
               auto ok = conn->executeVoid("BEGIN");
               return ok;
            }
         }
         catch(const std::exception& e)
         {
            Logger::logT("[sql] exec BEGIN, error: {}", e.what());
         }
         return false;
      }, _sqlConnPtrVariant
   );
}

bool DatabaseConnector::commitTransaction()
{
   return std::visit(
      [&](auto& conn)
      {
         try
         {
            if (conn->backendType() == _dbOption.dbDriver) 
            {
               auto ok = conn->executeVoid("COMMIT");
               return ok;
            }
         }
         catch(const std::exception& e)
         {
            Logger::logT("[sql] exec COMMIT error: {}", e.what());
         }
         return false;
      }, _sqlConnPtrVariant
   );
}

bool DatabaseConnector::rollbackTransaction()
{
   return std::visit(
      [&](auto& conn)
      {
         try
         {
            if (conn->backendType() == _dbOption.dbDriver) 
            {
               auto ok = conn->executeVoid("ROLLBACK");
               return ok;
            }
         }
         catch(const std::exception& e)
         {
            Logger::logT("[sql] exec ROLLBACK, error: {}", e.what());
         }
         return false;
      }, _sqlConnPtrVariant
   );
}

sql::SqlConnectionPtrVariant& DatabaseConnector::sqlConnPtrVariant()
{
   return _sqlConnPtrVariant;
}

sql::SqlDriverVariant& DatabaseConnector::getSqlDriverVariant()
{
   return _sqlDriverVariant;
}

sql::conf::ConnectorOption DatabaseConnector::option()
{
   return _dbConnOption;
}

} // namespace sql
} // namespace tbs