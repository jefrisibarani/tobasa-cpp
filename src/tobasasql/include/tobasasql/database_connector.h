#pragma once

#include "tobasasql/sql_service_base.h"
#include "tobasasql/settings.h"
#include "tobasasql/database_connector_base.h"

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 * Database connector
 */
class DatabaseConnector : public DatabaseConnectorBase
{
protected:
   sql::SqlDriverVariant                     _sqlDriverVariant;
   sql::SqlConnectionPtrVariant              _sqlConnPtrVariant;

#if defined(TOBASA_SQL_USE_PGSQL)
   sql::PgsqlDriver                          _pgsqlDriver;
   sql::SqlConnectionPtr<sql::PgsqlDriver>   _pPgsqlConn;
#endif

#if defined(TOBASA_SQL_USE_SQLITE)   
   sql::SqliteDriver                         _sqliteDriver;
   sql::SqlConnectionPtr<sql::SqliteDriver>  _pSqliteConn;
#endif

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
   sql::AdodbDriver                          _adodbDriver;
   sql::SqlConnectionPtr<sql::AdodbDriver>   _pAdodbConn;
#endif

#if defined(TOBASA_SQL_USE_ODBC)
   sql::OdbcDriver                           _odbcDriver;
   sql::SqlConnectionPtr<sql::OdbcDriver>    _pOdbcConn;
#endif

#if defined(TOBASA_SQL_USE_MYSQL)
   sql::MysqlDriver                          _mysqlDriver;
   sql::SqlConnectionPtr<sql::MysqlDriver>   _pMysqlConn;   
#endif

   sql::conf::ConnectorOption                _dbConnOption;
   sql::conf::Database                       _dbOption;

   /// List of SqlService created with this connector;
   std::vector<sql::SqlServiceInfo>          _services;

   void setSqlServiceCallbacks(SqlServicePtr svc)
   {
      svc->beginTransactionCallback    = [this](){ return beginTransaction(); };
      svc->commitTransactionCallback   = [this](){ return commitTransaction(); };
      svc->rollbackTransactionCallback = [this](){ return rollbackTransaction(); };
      svc->databaseConnectedCallback   = [this](){ return connected(); };
   }

public:

   DatabaseConnector(const sql::conf::ConnectorOption& dbConnOption, const std::string& name);
   virtual ~DatabaseConnector();

   /// Connect to database using settings provided ConnectorOption object.
   virtual bool connect();
   
   /// Disconnect sql connection.
   virtual bool disconnect();
   
   /// Initialize SQL driver.
   virtual void initSqlDriver();
   
   /// Check connection state .
   virtual bool connected();
   
   /// Test connection .
   virtual bool testConnection();

   /// Get sql connection pointer variant.
   virtual sql::SqlConnectionPtrVariant& sqlConnPtrVariant();
   
   /// Get sql driver variant.
   virtual sql::SqlDriverVariant& getSqlDriverVariant();

   sql::conf::ConnectorOption option();

   virtual bool beginTransaction();
   virtual bool commitTransaction();
   virtual bool rollbackTransaction();

   /// Create SqlService object.
   template< template <class> class ServiceType >
   SqlServicePtr createService()
   {
      using namespace sql;

      switch (_dbOption.dbDriver)
      {

#if defined(TOBASA_SQL_USE_SQLITE)
      case BackendType::sqlite:
      {
         SqlServicePtr svc = std::make_shared<ServiceType<SqliteDriver>>(*_pSqliteConn);
         setSqlServiceCallbacks(svc);
         _services.emplace_back(svc->info());

         return svc;
      }
#endif

#if defined(TOBASA_SQL_USE_PGSQL)
      case BackendType::pgsql:
      {
         SqlServicePtr svc = std::make_shared<ServiceType<PgsqlDriver>>(*_pPgsqlConn);
         setSqlServiceCallbacks(svc);
         _services.emplace_back(svc->info());
         return svc;
      }
#endif

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
      case BackendType::adodb:
      {
         SqlServicePtr svc = std::make_shared<ServiceType<AdodbDriver>>(*_pAdodbConn);
         setSqlServiceCallbacks(svc);
         _services.emplace_back(svc->info());
         return svc;
      }
#endif
      
#if defined(TOBASA_SQL_USE_ODBC)
      case BackendType::odbc:
      {
         SqlServicePtr svc = std::make_shared<ServiceType<OdbcDriver>>(*_pOdbcConn);
         setSqlServiceCallbacks(svc);
         _services.emplace_back(svc->info());
         return svc;
      }
#endif
      
#if defined(TOBASA_SQL_USE_MYSQL)
      case BackendType::mysql:
      {
         SqlServicePtr svc = std::make_shared<ServiceType<MysqlDriver>>(*_pMysqlConn);
         setSqlServiceCallbacks(svc);
         _services.emplace_back(svc->info());
         return svc;
      }
#endif

      default:
         throw AppException("Unknown SQL driver type");
      }
      
      return nullptr;
   }

};

} // namespace sql
} // namespace tbs