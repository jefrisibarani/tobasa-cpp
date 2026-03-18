#include <tobasa/format.h>
#include <tobasa/logger.h>
#include "tobasasql/exception.h"
#include "tobasasql/pgsql_type.h"
#include "tobasasql/pgsql_util.h"
#include "tobasasql/pgsql_connection.h"

namespace tbs {
namespace sql {

PgsqlConnection::PgsqlConnection()
    : ConnectionCommon()
{
   _pPGconn           = nullptr;
   _lastSystemOID     = 0;
   _dbOid             = 0;
   _needColumnQuoting = false;
   notifierSource     = "PgsqlConnection";
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

      // Setup client encoding
      std::string sql = "SET DateStyle=ISO; SELECT oid, pg_encoding_to_char(encoding) AS encoding, \n"
                        "datlastsysoid FROM pg_database WHERE ";

      // Note, can't use qtDbString here as we don't know the server version yet.
      std::string dbName(PQdb(_pPGconn));
      sql += "datname='" + dbName + "'";

      PGresult* qryRes = PQexec(_pPGconn, sql.c_str());
      if (qryRes == nullptr)
      {
         onNotifyWarning(logId() + "Could not set client encoding");

         // TODO_JEFRI : Should we stop here?
         //_connStatus = ConnectionStatus::bad;
         //disconnect();
         //throw tbs::SqlException("Could not set client encoding", "pgconn");
      }

      ExecStatusType status = PQresultStatus(qryRes);
      if ((status == PGRES_TUPLES_OK) || (status == PGRES_COMMAND_OK))
      {
         if (PQntuples(qryRes) > 0)
         {
            enum
            {
               F_OID = 0,
               F_ENC = 1,
               F_LASTSYSOID = 2
            };

            // setup column quoting
            std::string field2Name = PQfname(qryRes,F_LASTSYSOID);
            int field2Pos = PQfnumber(qryRes, "\"datlastsysoid\"");
            if (field2Pos >= 0) {
                  //setNeedColumnQuoting();
            }

            try
            {
               std::string oid            = PQgetvalue(qryRes, 0, F_OID);
               std::string encoding       = PQgetvalue(qryRes, 0, F_ENC);
               std::string datlastsysoid  = PQgetvalue(qryRes, 0, F_LASTSYSOID);

               _dbOid         = std::stol(oid);
               _lastSystemOID = std::stol(datlastsysoid);

               if (encoding != "SQL_ASCII" && encoding != "MULE_INTERNAL") {
                  encoding = "UTF8";
               }

               onNotifyDebug(tbsfmt::format("Setting client_encoding to '{}'", encoding));

               if (PQsetClientEncoding(_pPGconn, encoding.c_str()) == -1)
               {
                  onNotifyWarning(logId() + lastBackendError(), "pgconn");
                  // TODO_JEFRI : Should we stop here?
                  //_connStatus = ConnectionStatus::bad;
                  //disconnect();
                  //throw tbs::SqlException("Could not set client encoding", "PgsqlConnection");
               }
            }
            catch (std::exception & e)
            {
               std::string errMsg = tbsfmt::format("Error on connect: {}", e.what());
               onNotifyError(logId() + errMsg);

               _connStatus = ConnectionStatus::bad;
               disconnect();

               throw tbs::SqlException(errMsg, "PgsqlConnection");
            }
         }
         else
         {
            onNotifyWarning(logId() + "Could not set client encoding");
            // TODO_JEFRI : Should we stop here?
            //_connStatus = ConnectionStatus::bad;
            //disconnect();
            //throw tbs::SqlException("Could not set client encoding", "PgsqlConnection");
         }
      }

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

int PgsqlConnection::execute(
   const std::string& sql,
   const SqlParameterCollection& parameters)
{
   if (status() != ConnectionStatus::ok)
      return -1;

   if (logSqlQuery())
      onNotifyDebug(logId() + tbsfmt::format("execute: {}", sql));

   PGresult* qryRes = nullptr;
   ExecStatusType status;

   if (parameters.size() > 0)
      qryRes = executeParams(sql, parameters);
   else
      qryRes = PQexec(_pPGconn, sql.c_str());

   if (qryRes == nullptr)
   {
      std::string errmsg = tbsfmt::format("execute, {}", lastBackendError());
      onNotifyError(logId() + errmsg);
      throw tbs::SqlException(errmsg, "PgsqlConnection");
   }

   status = PQresultStatus(qryRes);
   if ((status == PGRES_TUPLES_OK) || (status == PGRES_COMMAND_OK))
   {
      // SELECT, CREATE TABLE AS, INSERT, UPDATE, DELETE, MOVE, FETCH, or COPY statement,
      // or an EXECUTE of a prepared query that contains an INSERT, UPDATE, or DELETE statement
      int affectedRows = 0;
      char* affRow = PQcmdTuples(qryRes);
      if (*affRow)
      {
         int  affRowI = atoi(affRow);
         affectedRows = (affRowI < 0) ? 0 : affRowI;
      }

      // clear the result
      PQclear(qryRes);

      if (logExecuteStatus()) 
         onNotifyTrace(logId() + tbsfmt::format("SQL command executed successfully, affectedRows: {}", affectedRows));

      return affectedRows;
   }
   else
   {
      // PGRES_EMPTY_QUERY, PGRES_COPY_OUT, PGRES_COPY_IN, PGRES_BAD_RESPONSE,
      // PGRES_FATAL_ERROR, PGRES_COPY_BOTH

      // clear the result
      PQclear(qryRes);

      std::string errmsg = tbsfmt::format("execute, {}", lastBackendError());
      onNotifyError(logId() + errmsg);
      throw tbs::SqlException(errmsg, "PgsqlConnection");
   }

   return -1;
}

std::string PgsqlConnection::executeScalar(
   const std::string& sql,
   const SqlParameterCollection& parameters)
{
   if (status() != ConnectionStatus::ok)
      throw tbs::SqlException("Invalid connection status", "PgsqlConnection");

   if (logSqlQuery())
      onNotifyDebug(logId() + tbsfmt::format("executeScalar: {}", sql));

   PGresult* qryRes = nullptr;
   ExecStatusType status;

   if (parameters.size() > 0) 
      qryRes = executeParams(sql, parameters);
   else 
      qryRes = PQexec(_pPGconn, sql.c_str());

   if (qryRes == nullptr)
   {
      std::string errmsg = tbsfmt::format("execute, {}", lastBackendError());
      onNotifyError(logId() + errmsg);
      throw tbs::SqlException(errmsg, "PgsqlConnection");
   }

   status = PQresultStatus(qryRes);
   if ((status == PGRES_TUPLES_OK) || (status == PGRES_COMMAND_OK))
   {
      // PGRES_COMMAND_OK is for commands that can never return rows (INSERT or UPDATE without a RETURNING clause, etc.)
      // successfull SELECT query returning no row result status from backend is PGRES_TUPLES_OK

      std::string result;

      // Check for a returned row
      if (PQntuples(qryRes) < 1)
      {
         // Scalar query returned no record
         onNotifyDebug(logId() + "Scalar query returned no record");
         result = "";
      }
      else
      {
         // Retrieve the query result and return it.
         result = PQgetvalue(qryRes, 0, 0);
      }

      // clear the result
      PQclear(qryRes);

      return result;
   }
   else
   {
      // PGRES_EMPTY_QUERY, PGRES_COPY_OUT, PGRES_COPY_IN, PGRES_BAD_RESPONSE,
      // PGRES_FATAL_ERROR, PGRES_COPY_BOTH

      // clear the result
      PQclear(qryRes);

      std::string errmsg = tbsfmt::format("execute, {}", lastBackendError());
      onNotifyError(logId() + errmsg);
      throw tbs::SqlException(errmsg, "PgsqlConnection");
   }

   return "";
}

std::string PgsqlConnection::versionString()
{
   if (status() != ConnectionStatus::ok)
      return "";

   SqlApplyLogInternal applyLogRule(this);   

   auto libpqVersion = PQlibVersion();
   auto backendVersion = executeScalar("select version()");
   return backendVersion + std::string(", Libpq version: ") + std::to_string(static_cast<int>(libpqVersion));
}

std::string PgsqlConnection::databaseName()
{
   if (status() != ConnectionStatus::ok)
      return "";
   
   SqlApplyLogInternal applyLogRule(this);
   return executeScalar("SELECT current_database()");
}

BackendType PgsqlConnection::backendType() const { return BackendType::pgsql; }

std::string PgsqlConnection::dbmsName() { return name(); }

int64_t PgsqlConnection::lastInsertRowid()
{
   throw SqlException("PgsqlConnection does not support lastInsertRowid()", "PgsqlConnection");
}

// -------------------------------------------------------
// Specific implementation functions
// -------------------------------------------------------

std::string PgsqlConnection::lastBackendError()
{
   std::string errmsg;
   char* err = nullptr;
   if (_pPGconn != nullptr)
   {
      err = PQerrorMessage(_pPGconn);
      if (err != nullptr)
      {
         errmsg = std::string(err);
         if (errmsg.empty())
            errmsg = "";
      }
   }
   return errmsg;
}

PGconn* PgsqlConnection::nativeConn() const { return _pPGconn; }

int PgsqlConnection::transactionStatus()
{
   return PQtransactionStatus(_pPGconn);
}

void PgsqlConnection::pgNoticeProcessor(void* arg, const char* message)
{
   ((PgsqlConnection*)arg)->processNotice(message);
}

PGresult* PgsqlConnection::executeParams(const std::string& sql, const SqlParameterCollection& parameters)
{
   int    totalParam    = (int)parameters.size();
   Oid*   paramTypes    = new Oid[totalParam];
   char** paramValues   = new char* [totalParam];
   int*   paramLengths  = new int[totalParam];
   int*   paramFormats  = new int[totalParam];

   try
   {
      PGresult* qryRes = nullptr;

      for (unsigned int i = 0; i < parameters.size(); i++)
      {
         auto param = parameters.at(i);
         PgsqlType pgtype = pgsqlDataTypeFromDataType(param->type());
         paramTypes[i] = static_cast<Oid>(pgtype);

         /*
         std::string value    = tbs::sql::util::variantToString(param->value());
         char* valueChar      = static_cast<char*>((char*)value.c_str());
         char* valueCharFinal = (char*)malloc(strlen(valueChar)+1);
         if (valueCharFinal!= nullptr)
            strcpy(valueCharFinal, valueChar);

         paramValues[i] = valueCharFinal;
         */

         if (std::holds_alternative<std::monostate>(param->value()))
         {
            paramValues[i]  = nullptr;
         }
         else
         {
            paramValues[i] = *(param->valueCharPtr(

               [&](std::string& value)
               {
                  if (param->type() == DataType::varbinary)
                     value = "\\x" + value;
               }
            ));
         }
         
         paramLengths[i] = static_cast<int>(param->size());
         paramFormats[i] = 0;

      }

      qryRes = PQexecParams(
                  _pPGconn,      // PGconn *conn
                  sql.c_str(),   // const char *command
                  totalParam,    // const Oid *paramTypes
                  paramTypes,    // const char * const *paramValues
                  paramValues,   // const int *paramLengths
                  paramLengths,  // const int *paramLengths
                  paramFormats,  // const int *paramFormats
                  0);            // int resultFormat

      delete[] paramTypes;
      delete[] paramValues;
      delete[] paramLengths;
      delete[] paramFormats;

      return qryRes;
   }
   catch (const tbs::TypeException &ex)
   {
      delete[] paramTypes;
      delete[] paramValues;
      delete[] paramLengths;
      delete[] paramFormats;

      throw tbs::SqlException(tbsfmt::format("executeParams, {}", ex.what()), "PgsqlConnection");
   }

   return nullptr;
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