#include <atomic>
#include <iostream>
#include <tobasa/notifier.h>
#include <tobasa/variant_helper.h>
#include "tobasasql/sql_dataset.h"
#include "tobasasql/sql_util.h"
#include "tobasasql/pgsql_type.h"
#include "tobasasql/pgsql_util.h"
#include "tobasasql/pgsql_connection.h"
#include "tobasasql/pgsql_result.h"
#include "tobasasql/pgsql_command.h"

namespace tbs {
namespace sql {

namespace {

std::atomic_uint64_t nextStatementId{0};

/**
 * Generate PostgreSQL statement name
 * Statement name must start with a letter or underscore, 
 * and can contain only letters, numbers, and underscores.
 * Do not use spaces or special characters such as -, ., /, or ;.
 */
std::string makeStatementName()
{
   return "tbs_sql_stmt_" + std::to_string(++nextStatementId);
}

} // namespace

PgsqlCommand::PgsqlCommand(PgsqlConnection* conn)
   : _pConn(conn)
   , _pPGconn(nullptr)
   , _affectedRows(-1)
{
   if (_pConn)
      _pPGconn = _pConn->nativeConnection();

   notifierSource = "PgsqlCommand";
}

PgsqlCommand::~PgsqlCommand()
{
   close();
}

bool PgsqlCommand::query(const std::string& sql, const SqlParameterCollection& parameters)
{
   if (!prepare(sql))
      return false;

   return bind(parameters);
}

bool PgsqlCommand::doPrepare(const std::string& sql, int totalParameter)
{
   if (_prepared)
      return true;

   _statementName   = makeStatementName();
   _parameterCount  = totalParameter;
   PGresult* qryRes = PQprepare(
                        _pPGconn,               // PGconn *conn
                        _statementName.c_str(), // const char *stmtName
                        sql.c_str(),            // const char *query
                        _parameterCount,        // int nParams
                        nullptr                 // const Oid *paramTypes
                     );

   if (qryRes == nullptr)
      throw SqlException(lastBackendError(), "PgsqlCommand");

   if (PQresultStatus(qryRes) != PGRES_COMMAND_OK)
   {
      _hasBackendError = true;
      std::string errorMsg(PQerrorMessage(_pPGconn));
      if (qryRes != nullptr)
         PQclear(qryRes);

      onNotifyError(_pConn->logId() + " doPrepare, " + errorMsg, "PgsqlCommand");
      throw SqlException(errorMsg, "PgsqlCommand");
   }

   if (qryRes==nullptr)
      PQclear(qryRes);

   _prepared = true;
   return true;
}

bool PgsqlCommand::prepare(const std::string& sql)
{
   if (_pPGconn == nullptr)
      throw SqlException("Invalid connection object", "PgsqlCommand");

   close();
   _prepared = false;
   _sql = sql;
   return true;
}

bool PgsqlCommand::bind(const SqlParameterCollection& parameters)
{
   if (_pPGconn == nullptr)
      throw SqlException("Invalid connection object", "PgsqlCommand");

   onNotifyTrace(_pConn->logId() + "SqliteCommand, Binding paramaters" );

   int totalParam  = static_cast<int>(parameters.size());
   if (totalParam > 0)
   {
      if ( ! doPrepare(_sql, totalParam))
         return false;

      if (_statementName.empty())
         return false;

      _pgParam.initialize(totalParam);

      try
      {
         for (size_t i = 0; i < parameters.size(); i++)
         {
            auto param = parameters.at(i);
            PgsqlType pgtype = pgsqlDataTypeFromDataType(param->type());
            _pgParam.types[i] = static_cast<Oid>(pgtype);

            if (std::holds_alternative<std::monostate>(param->value()))
            {
               _pgParam.values[i] = nullptr;
            }
            else
            {
               _pgParam.values[i] = *(param->valueCharPtr(

                  [&](std::string& value)
                  {
                     if (param->type() == DataType::varbinary)
                        value = "\\x" + value;
                     if (param->type() == DataType::varbit)
                     {
                        // nothing to do here. data already in bit-string format. e.g 00001100
                        auto x=1;
                     }
                  }
               ));
            }
            
            _pgParam.lengths[i] = static_cast<int>(param->size());
            _pgParam.formats[i] = 0;
         }

         return true;
      }
      catch (const tbs::TypeException &ex)
      {
         _pgParam.reset();
         throw tbs::SqlException(tbsfmt::format("bind, {}", ex.what()), "PgsqlCommand");
      }
   }
   else
   {
      // empty parameters supplied
      _parameterCount = 0;
      return true;
   }

   return false;
}

void PgsqlCommand::reset()
{
   if (_pPGconn == nullptr)
      throw SqlException("Invalid connection object", "PgsqlCommand");

   _pgParam.reset();
   _affectedRows = -1;
   _hasBackendError = false;
}

void PgsqlCommand::close()
{
   if (!_statementName.empty())
      removeStatement();

   _statementName.clear();
   _sql.clear();
   _parameterCount = 0;
   _prepared = false;
   _pgParam.reset();
   _affectedRows = -1;
}

int PgsqlCommand::execute()
{
   if (_pPGconn == nullptr)
      throw tbs::SqlException("Invalid connection object", "PgsqlCommand");

   if (_pConn->logSqlQuery())
      onNotifyDebug( _pConn->logId() + tbsfmt::format("execute: {}", _sql) );

   PGresult* qryRes = nullptr;
   if (_parameterCount == 0)
   {
      qryRes = PQexec(_pPGconn, _sql.c_str());
   }
   else
   {
      qryRes = PQexecPrepared(
                  _pPGconn,
                  _statementName.c_str(), // const char *stmtName
                  _pgParam.total,         // int nParams
                  _pgParam.values,        // const char * const *paramValues
                  _pgParam.lengths,       // const int *paramLengths
                  _pgParam.formats,       // const int *paramFormats
                  0                       // int resultFormat
               );
   }

   if (qryRes == nullptr)
   {
      std::string errmsg = tbsfmt::format("execute, {}", lastBackendError());
      onNotifyError(_pConn->logId() + errmsg);
      throw tbs::SqlException(errmsg, "PgsqlCommand");
   }

   ExecStatusType status = PQresultStatus(qryRes);
   if ((status == PGRES_TUPLES_OK) || (status == PGRES_COMMAND_OK))
   {
      // SELECT, CREATE TABLE AS, INSERT, UPDATE, DELETE, MOVE, FETCH, or COPY statement,
      // or an EXECUTE of a prepared query that contains an INSERT, UPDATE, or DELETE statement
      // Row-producing statements such as SELECT do not affect rows.
      if (status == PGRES_COMMAND_OK)
         _affectedRows = getAffectedRows(qryRes);
      else
         _affectedRows = 0;

      // clear the result
      PQclear(qryRes);

      if (_pConn->logExecuteStatus()) 
         onNotifyDebug(_pConn->logId() + tbsfmt::format("SQL command executed successfully, affectedRows: {}", _affectedRows));

      return _affectedRows;
   }
   else
   {
      // PGRES_EMPTY_QUERY, PGRES_COPY_OUT, PGRES_COPY_IN, PGRES_BAD_RESPONSE,
      // PGRES_FATAL_ERROR, PGRES_COPY_BOTH
      std::string errMsg = PQresultErrorMessage(qryRes);
      PQclear(qryRes); // clear the result
      errMsg = tbsfmt::format("execute, {}", errMsg );
      onNotifyError(_pConn->logId() + errMsg);
      throw SqlException(errMsg, "PgsqlCommand");
   }

   return -1; // should not reach here
}

std::string PgsqlCommand::executeScalar()
{
   if (_pPGconn == nullptr)
      throw SqlException("Invalid connection object", "PgsqlCommand");

   if (_pConn->logSqlQuery())
      onNotifyDebug(_pConn->logId() + tbsfmt::format("executeScalar: {}", _sql));

   PGresult* qryRes = nullptr;
   if (_parameterCount == 0)
   {
      qryRes = PQexec(_pPGconn, _sql.c_str());
   }
   else
   {
      qryRes = PQexecPrepared(
                  _pPGconn,
                  _statementName.c_str(), // const char *stmtName
                  _pgParam.total,         // int nParams
                  _pgParam.values,        // const char * const *paramValues
                  _pgParam.lengths,       // const int *paramLengths
                  _pgParam.formats,       // const int *paramFormats
                  0                       // int resultFormat
               );
   }

   if (qryRes == nullptr)
   {
      std::string errmsg = tbsfmt::format("executeScalar, {}", lastBackendError());
      onNotifyError(_pConn->logId() + errmsg);
      throw SqlException(errmsg, "PgsqlCommand");
   }

   ExecStatusType status = PQresultStatus(qryRes);
   if ((status == PGRES_TUPLES_OK) || (status == PGRES_COMMAND_OK))
   {
      // PGRES_COMMAND_OK is for commands that can never return rows (INSERT or UPDATE without a RETURNING clause, etc.)
      // successfull SELECT query returning no row result status from backend is PGRES_TUPLES_OK
      if (status == PGRES_COMMAND_OK)
         _affectedRows = getAffectedRows(qryRes);
      else
         _affectedRows = 0;

      std::string result;
      std::string logMsg;

      // Check for a returned row
      if (PQntuples(qryRes) < 1)
      {
         // Scalar query returned no record
         result = "";
         logMsg = "returned no record";
      }
      else
      {
         // Retrieve the query result and return it.
         if (PQgetisnull(qryRes, 0, 0))
         {
            result = sql::NULLSTR;
            logMsg = "returned SQL NULL";
         }
         else
            result = PQgetvalue(qryRes, 0, 0);
      }

      PQclear(qryRes); // clear the result

      if (_pConn->logExecuteStatus())
         onNotifyDebug(_pConn->logId() + tbsfmt::format("Scalar query executed successfully {}", logMsg));

      return result;
   }
   else
   {
      // PGRES_EMPTY_QUERY, PGRES_COPY_OUT, PGRES_COPY_IN, PGRES_BAD_RESPONSE,
      // PGRES_FATAL_ERROR, PGRES_COPY_BOTH
      std::string errmsg = PQresultErrorMessage(qryRes);
      PQclear(qryRes); // clear the result
      errmsg = tbsfmt::format("execute, {}", errmsg );
      onNotifyError(_pConn->logId() + errmsg);
      throw SqlException(errmsg, "PgsqlCommand");
   }

   return {}; // should not reach here
}

PgsqlCommand::DataSetPtr PgsqlCommand::executeResult()
{
   // Let executePgResult() get pg result and do all error check

   PGresult* qryRes = executePgResult();
   if (qryRes == nullptr)
      return nullptr;

   int nRows    = 0;
   int nCols    = 0;
   auto dataSet = std::make_shared<DataSet<VariantType>>();

   ExecStatusType status = PQresultStatus(qryRes);
   if ((status == PGRES_TUPLES_OK) || (status == PGRES_COMMAND_OK))
   {
      // PGRES_COMMAND_OK is for commands that can never return rows (INSERT or UPDATE without a RETURNING clause, etc.)
      // successfull SELECT query returning no row result status from backend is PGRES_TUPLES_OK

      // SELECT, CREATE TABLE AS, INSERT, UPDATE, DELETE, MOVE, FETCH, or COPY statement,
      // or an EXECUTE of a prepared query that contains an INSERT, UPDATE, or DELETE statement

      if (status == PGRES_COMMAND_OK)
      {
         _affectedRows = getAffectedRows(qryRes);
         nCols = 0;
         nRows = 0;
      }
      if (status == PGRES_TUPLES_OK)
      {
         _affectedRows = 0;
         // get total columns and rows
         nRows = PQntuples(qryRes);
         nCols = PQnfields(qryRes);
      
         for (int row = 0; row < nRows; ++row)
         {
            auto& resultRow = dataSet->addRow();

            for (int column = 0; column < nCols; ++column)
            {
               int isnull        = PQgetisnull(qryRes, row, column);
               const char* value = PQgetvalue(qryRes,  row, column);

               if (isnull == 1)
                  resultRow.emplace_back(std::monostate{});
               else
                  resultRow.emplace_back(value == nullptr ? std::string{} : reinterpret_cast<const char*>(value));
            }
         }
      }

      if (_pConn->logExecuteStatus()) 
         onNotifyDebug(_pConn->logId() + tbsfmt::format("SQL command executed successfully, row: {} column: {}, affectedRows: {}", nRows, nCols, _affectedRows));

      PQclear(qryRes);
   }

   dataSet->totalColumns(nCols);

   return dataSet;
}

PgsqlResult PgsqlCommand::executeSqlResult()
{
   if (_pPGconn == nullptr)
      throw SqlException("Invalid connection object", "PgsqlCommand");
      
   PgsqlResult result;
   result.connection(_pConn);
   result.runPreparedQuery(*this);

   return std::move(result);
}

PGresult* PgsqlCommand::executePgResult()
{
   if (_pPGconn == nullptr)
      throw tbs::SqlException("Invalid connection object", "PgsqlCommand");

   if (_pConn->logSqlQuery())
      onNotifyDebug(_pConn->logId() + tbsfmt::format("executePgResult: {}", _sql));

   PGresult* qryRes = nullptr;
   if (_parameterCount == 0)
   {
      qryRes = PQexec(_pPGconn, _sql.c_str());
   }
   else
   {
      qryRes = PQexecPrepared(
                  _pPGconn,
                  _statementName.c_str(), // const char *stmtName
                  _pgParam.total,         // int nParams
                  _pgParam.values,        // const char * const *paramValues
                  _pgParam.lengths,       // const int *paramLengths
                  _pgParam.formats,       // const int *paramFormats
                  0                       // int resultFormat
               );
   }

   if (qryRes == nullptr)
   {
      std::string errmsg = tbsfmt::format("executePgResult, {}", lastBackendError());
      onNotifyError(_pConn->logId() + errmsg);
      throw tbs::SqlException(errmsg, "PgsqlCommand");
   }

   ExecStatusType status = PQresultStatus(qryRes);
   if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK)
   {
      // PGRES_EMPTY_QUERY, PGRES_COPY_OUT, PGRES_COPY_IN, PGRES_BAD_RESPONSE,
      // PGRES_FATAL_ERROR, PGRES_COPY_BOTH
      if (status == PGRES_COMMAND_OK)
         _affectedRows = getAffectedRows(qryRes);
      else
         _affectedRows = 0;

      std::string errmsg = PQresultErrorMessage(qryRes);
      PQclear(qryRes); // clear the result
      errmsg = tbsfmt::format("executePgResult, {}", errmsg );
      onNotifyError(_pConn->logId() + errmsg);
      throw SqlException(errmsg, "PgsqlCommand");
   }

   return qryRes;
} 

std::string PgsqlCommand::lastBackendError()
{
   if (_pPGconn == nullptr)
   {
      onNotifyError("lastBackendError, invalid connection object", "PgsqlCommand");
      return "invalid connection object";
   }

   const char* err = PQerrorMessage(_pPGconn);
   if (err != nullptr && err[0] != '\0')
      return std::string(err);

   return "Unknown PostgreSQL backend error";
}

int PgsqlCommand::getAffectedRows(PGresult* result)
{
   int affectedRows = 0;
   const char* value = PQcmdTuples(result);

   if (value != nullptr && *value != '\0')
      affectedRows = ::atoi(value);

   return affectedRows;
}

void PgsqlCommand::removeStatement()
{
   // NOTE_JEFRI: Do not throw inside this function
   // close() called this function, which is called from destructor()
   // When an exception is already being unwound, a second exception thrown 
   // from a destructor is treated as fatal. In C++, that leads to std::terminate() / abort

   if (_pPGconn == nullptr)
   {
      onNotifyError("removeStatement, invalid connection object");
      return;
   }

   if (_statementName.empty())
      return;

   if (!_hasBackendError)
   {
      const std::string deallocate = "DEALLOCATE " + _statementName;
      PGresult* result = PQexec(_pPGconn, deallocate.c_str());
      if (result == nullptr)
      {
         std::string errmsg = tbsfmt::format("removeStatement, {}", lastBackendError());
         onNotifyError(_pConn->logId() + errmsg);
         return;
      }

      ExecStatusType status = PQresultStatus(result);
      if (status != PGRES_COMMAND_OK)
      {  
         // PQTRANS_IDLE PQTRANS_ACTIVE  PQTRANS_INTRANS  PQTRANS_INERROR  PQTRANS_UNKNOWN
         _statementName.clear();
         std::string errmsg = PQresultErrorMessage(result);
         if ( result != nullptr)
            PQclear(result);

         errmsg = tbsfmt::format("removeStatement, {}", errmsg );
         onNotifyError(_pConn->logId() + errmsg);
         return;
      }

      if (result != nullptr)
         PQclear(result);
   }
}

PGresult* PgsqlCommand::executeParams(const std::string& sql, const SqlParameterCollection& parameters)
{
   int totalParam  = (int)parameters.size();
   PgsqlParameterContext pgParam(totalParam);

   try
   {
      PGresult* qryRes = nullptr;

      for (unsigned int i = 0; i < parameters.size(); i++)
      {
         auto param = parameters.at(i);
         PgsqlType pgtype = pgsqlDataTypeFromDataType(param->type());
         pgParam.types[i] = static_cast<Oid>(pgtype);

         if (std::holds_alternative<std::monostate>(param->value()))
         {
            pgParam.values[i] = nullptr;
         }
         else
         {
            pgParam.values[i] = *(param->valueCharPtr(

               [&](std::string& value)
               {
                  if (param->type() == DataType::varbinary)
                     value = "\\x" + value;
                  if (param->type() == DataType::varbit)
                  {
                     // nothing to do here. data already in bit-string format. e.g 00001100
                     auto x=1;
                  }
               }
            ));
         }
         
         pgParam.lengths[i] = static_cast<int>(param->size());
         pgParam.formats[i] = 0;
      }

      qryRes = PQexecParams(
                  _pPGconn,         // PGconn *conn
                  sql.c_str(),      // const char *command
                  pgParam.total,    // int nParams
                  pgParam.types,    // const Oid *paramTypes
                  pgParam.values,   // const char * const *paramValues
                  pgParam.lengths,  // const int *paramLengths
                  pgParam.formats,  // const int *paramFormats
                  0                 // int resultFormat
               );

      pgParam.reset();
      return qryRes;
   }
   catch (const tbs::TypeException &ex)
   {
      pgParam.reset();
      throw tbs::SqlException(tbsfmt::format("executeParams, {}", ex.what()), "PgsqlCommand");
   }

   return nullptr;
}


} // namespace sql
} // namespace tbs