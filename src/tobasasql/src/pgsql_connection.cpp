#include <tobasa/format.h>
#include <tobasa/logger.h>
#include "tobasasql/exception.h"
#include "tobasasql/pgsql_type.h"
#include "tobasasql/pgsql_util.h"
#include "tobasasql/pgsql_command.h"
#include "tobasasql/pgsql_connection.h"
#include "tobasasql/pgsql_parameter.h"

namespace tbs {
namespace sql {

PgsqlConnection::PgsqlConnection()
    : ConnectionCommon()
    , _pPGconn(nullptr)
    , _lastSystemOID(0)
    , _dbOid(0)
    , _needColumnQuoting(false)
{
   notifierSource     = "PgsqlConnection";
}

PgsqlConnection::PgsqlConnection(PgsqlConnection&& other) noexcept
   : ConnectionCommon(std::move(other))
   , _pPGconn(          other._pPGconn)
   , _lastSystemOID(    other._lastSystemOID)
   , _dbOid(            other._dbOid)
   , _needColumnQuoting(other._needColumnQuoting)
{
   other._pPGconn           = nullptr;
   other._lastSystemOID     = 0;
   other._dbOid             = 0;
   other._needColumnQuoting = false;
}

PgsqlConnection& PgsqlConnection::operator=(PgsqlConnection&& other) noexcept
{
   if (this != &other)
   {
      disconnect();
      ConnectionCommon::operator=(std::move(other));

      _pPGconn           = other._pPGconn;
      _lastSystemOID     = other._lastSystemOID;
      _dbOid             = other._dbOid;
      _needColumnQuoting = other._needColumnQuoting;

      other._pPGconn           = nullptr;
      other._lastSystemOID     = 0;
      other._dbOid             = 0;
      other._needColumnQuoting = false;
   }
   return *this;
}

PgsqlConnection::~PgsqlConnection()
{
   disconnect();
}

std::string PgsqlConnection::name() const
{
   return "PostgreSQL Connection";
}

bool PgsqlConnection::connect(const std::string& connString)
{
   
   if (status() == ConnectionStatus::ok)
      return true;
   else
      disconnect();  // if we got broken connection, disconnect first, to reset _pPGconn

   _pPGconn = PQconnectdb(connString.c_str());
   ConnStatusType connStatus = PQstatus(_pPGconn);

   if (connStatus != CONNECTION_OK)
   {
      std::string errmsg(PQerrorMessage(_pPGconn));
      onNotifyError(logId() + errmsg);
      disconnect();
      throw SqlException(errmsg, "PgsqlConnection");
   }

   if (connStatus == CONNECTION_OK)
   {
      _connStatus = ConnectionStatus::ok;
      PQsetNoticeProcessor(_pPGconn, pgNoticeProcessor, this);

      if (PQisthreadsafe())
         onNotifyDebug(logId() + "Running thread-safe libpq");

      // Always use UTF-8 for the application before any text query is executed.
      if (PQsetClientEncoding(_pPGconn, "UTF8") == -1)
      {
         // Compatibility with old PostgreSQL naming
         if (PQsetClientEncoding(_pPGconn, "UNICODE") == -1)
         {
            std::string errMsg = lastBackendError();
            onNotifyWarning(logId() + "Failed setting client encoding: " + errMsg);
            //throw tbs::SqlException(errMsg, "PgsqlConnection");
         }
      }

      std::string dbEncoding;

      PGresult* qryRes = PQexec(_pPGconn,
         "SET DateStyle=ISO; "
         "SELECT oid, pg_encoding_to_char(encoding) AS encoding "
         "FROM pg_database WHERE datname = current_database() "
      );

      if (qryRes == nullptr)
      {
         std::string errMsg = lastBackendError();
         onNotifyError(logId() + "SQL Query failed: " + errMsg);
         throw tbs::SqlException(errMsg, "PgsqlConnection");
      }
      else if (PQresultStatus(qryRes) == PGRES_TUPLES_OK && PQntuples(qryRes) > 0)
      {
         _dbOid     = std::stol(PQgetvalue(qryRes, 0, 0));
         dbEncoding = PQgetvalue(qryRes, 0, 1);
      }
      else
      {
         std::string errMsg = PQresultErrorMessage(qryRes);
         onNotifyError(logId() + "Could not get database information: " + errMsg);
         PQclear(qryRes);
         throw tbs::SqlException(errMsg, "PgsqlConnection");
      }

      PQclear(qryRes);

      const char* clientEncoding = PQparameterStatus(_pPGconn, "client_encoding");

      onNotifyInfo(logId() + tbsfmt::format("PostgreSQL client_encoding = '{}'", clientEncoding ? clientEncoding : "<null>"));

      return true;
   }

   return false;
}

bool PgsqlConnection::disconnect()
{
   if (_pPGconn != nullptr)
   {
      PQfinish(_pPGconn);
      _pPGconn = nullptr;
      _connStatus = ConnectionStatus::bad;
   }

   return true;
}

ConnectionStatus PgsqlConnection::status()
{
   ConnectionStatus oldStatus = _connStatus;

   if (_pPGconn == nullptr)
   {
      _connStatus = ConnectionStatus::bad;
      return _connStatus;
   }

   if ( checkStatus() )
   {
      if (PQstatus(_pPGconn) == CONNECTION_OK)
         _connStatus = ConnectionStatus::ok;
      else if (PQstatus(_pPGconn) == CONNECTION_BAD)
      {
         // our last connection was ok
         if (oldStatus == ConnectionStatus::ok)
            _connStatus = ConnectionStatus::broken;
         else
            _connStatus = ConnectionStatus::bad;
      }
   }
   else 
   {
      _connStatus = ConnectionStatus::bad;
      throw tbs::SqlException("invalid connection object", "PgsqlConnection");
   }

   return _connStatus;
}

int PgsqlConnection::execute(const std::string& sql, const SqlParameterCollection& parameters)
{
   if (status() != ConnectionStatus::ok)
      return -1;

   PgsqlCommand cmd(this);
   if (cmd.query(sql, parameters))
      return cmd.execute();
   else
      return -1;
}

std::string PgsqlConnection::executeScalar(const std::string& sql, const SqlParameterCollection& parameters)
{
   if (status() != ConnectionStatus::ok)
      return "";

   PgsqlCommand cmd(this);
   if (cmd.query(sql, parameters))
      return cmd.executeScalar();
   else 
      return "";
}

std::string PgsqlConnection::versionString()
{
   if (status() != ConnectionStatus::ok)
      return {};

   SqlApplyLogInternal applyLogRule(this);   

   auto libpqVersion = PQlibVersion();
   auto backendVersion = executeScalar("select version()");
   return backendVersion + std::string(", Libpq version: ") + std::to_string(static_cast<int>(libpqVersion));
}

std::string PgsqlConnection::databaseName()
{
   if (status() != ConnectionStatus::ok)
      return {};
   
   SqlApplyLogInternal applyLogRule(this);
   return executeScalar("SELECT current_database()");
}

BackendType PgsqlConnection::backendType() const { return BackendType::pgsql; }

std::string PgsqlConnection::dbmsName() { return name(); }

bool PgsqlConnection::startTransaction()
{
   return execute("BEGIN") >= 0;
}

bool PgsqlConnection::commitTransaction()
{
   return execute("COMMIT") >= 0;
}

bool PgsqlConnection::rollbackTransaction()
{
   return execute("ROLLBACK") >= 0;
}

int64_t PgsqlConnection::lastInsertRowid()
{
   throw SqlException("PgsqlConnection does not support lastInsertRowid()", "PgsqlConnection");
}

// -------------------------------------------------------
// Specific implementation functions
// -------------------------------------------------------

std::string PgsqlConnection::lastBackendError()
{
   if (_pPGconn == nullptr)
   {
      onNotifyError("lastBackendError, invalid connection object", "PgsqlConnection");
      return "invalid connection object";
   }

   const char* err = PQerrorMessage(_pPGconn);
   if (err != nullptr && err[0] != '\0')
      return std::string(err);

   return "Unknown PostgreSQL backend error";
}

PGconn* PgsqlConnection::nativeConnection() const { return _pPGconn; }

int PgsqlConnection::transactionStatus()
{
   return PQtransactionStatus(_pPGconn);
}

void PgsqlConnection::pgNoticeProcessor(void* arg, const char* message)
{
   ((PgsqlConnection*)arg)->processNotice(message);
}

void PgsqlConnection::registerNoticeProcessor(PQnoticeProcessor proc, void* arg)
{
   PQsetNoticeProcessor(_pPGconn, proc, arg);
}

void PgsqlConnection::processNotice(const char* msg)
{
   std::string message(msg);
   onNotifyInfo(message);
}


} // namespace sql
} // namespace tbs