#include <cassert>
#include <string>
#include <cstring>
#include <memory>
#include <libpq-fe.h>
#include <tobasa/logger.h>
#include "tobasasql/pgsql_type.h"
#include "tobasasql/pgsql_util.h"
#include "tobasasql/pgsql_result.h"

namespace tbs {
namespace sql {

PgsqlResult::PgsqlResult(PgsqlConnection* pconn)
   : ResultCommon()
{
   _pPGresult      = nullptr;
   _pConn          = pconn;
   notifierSource  = "PgsqlResult";
   _navigator.init( std::bind(&PgsqlResult::totalRows, this) );
}

PgsqlResult::~PgsqlResult()
{
   if (_pPGresult != nullptr)
   {
      PQclear(_pPGresult);
      _pPGresult = nullptr ;
   }
}

// -------------------------------------------------------
// Specific implementation methods
// -------------------------------------------------------

std::string PgsqlResult::name() const
{
   return "Postgresql Result";
}

bool PgsqlResult::runQuery(const std::string& sql, const SqlParameterCollection& parameters)
{
   if (_pConn == nullptr)
      return false;

   if (_pConn->status() != ConnectionStatus::ok)
      return false;

   if (_optionOpenTable)
      _qryStr = "SELECT * FROM " + sql;
   else
      _qryStr = sql;

   if (_pConn->logSqlQuery())
      onNotifyDebug(_pConn->logId() + tbsfmt::format("runQuery: {}", _qryStr));

   ExecStatusType status;

   if (parameters.size() > 0)
      _pPGresult = _pConn->executeParams(_qryStr, parameters);
   else
      _pPGresult = PQexec(_pConn->nativeConn(), _qryStr.c_str());


   if (_pPGresult == nullptr)
   {
      std::string errmsg = tbsfmt::format("execute, {}", _pConn->lastBackendError());
      onNotifyError(_pConn->logId() + errmsg);
      throw tbs::SqlException(errmsg, "PgsqlResult");
   }

   status = PQresultStatus(_pPGresult);

   if ((status == PGRES_TUPLES_OK) || (status == PGRES_COMMAND_OK))
   {
      // PGRES_COMMAND_OK is for commands that can never return rows (INSERT or UPDATE without a RETURNING clause, etc.)
      // successfull SELECT query returning no row result status from backend is PGRES_TUPLES_OK

      // SELECT, CREATE TABLE AS, INSERT, UPDATE, DELETE, MOVE, FETCH, or COPY statement,
      // or an EXECUTE of a prepared query that contains an INSERT, UPDATE, or DELETE statement
      char* affRow = PQcmdTuples(_pPGresult);
      if (*affRow)
      {
         int affRowI = atoi(affRow);
         _affectedRows = (affRowI < 0) ? 0 : affRowI;
      }

      if (status != PGRES_TUPLES_OK)
      {
         _nColumns = 0;
         _nRows = 0;
         _navigator.moveFirst();

         if (_pConn->logExecuteStatus()) 
            onNotifyTrace(_pConn->logId() + tbsfmt::format("SQL command executed successfully, affectedRows: {}", _affectedRows));
      }
      else
      {
         // get total columns and rows
         _nRows    = PQntuples(_pPGresult);
         _nColumns = PQnfields(_pPGresult);

         if (_pConn->logExecuteStatus()) 
            onNotifyTrace(_pConn->logId() + tbsfmt::format("SQL command executed successfully, row: {} column: {}, affectedRows: {}", _nRows, _nColumns, _affectedRows));

         // we have total columns, now set up columns info
         setupColumnProperties();
      }

      if (_nRows > 0)
         _resultStatus = ResultStatus::tuplesOk;
      else
         _resultStatus = ResultStatus::commandOk;


      _navigator.moveFirst();

      return true;
   }
   else
   {
      // PGRES_EMPTY_QUERY, PGRES_COPY_OUT, PGRES_COPY_IN, PGRES_BAD_RESPONSE,
      // PGRES_FATAL_ERROR, PGRES_COPY_BOTH

      // clear the result
      PQclear(_pPGresult);
      _pPGresult = nullptr;

      std::string errmsg = tbsfmt::format("execute, {}", _pConn->lastBackendError());
      onNotifyError(_pConn->logId() + errmsg);
      throw tbs::SqlException(errmsg, "PgsqlResult");
   }

   return false;
}

void PgsqlResult::connection(PgsqlConnection* conn)
{
   _pConn = conn;
}

PgsqlConnection* PgsqlResult::connection() const { return _pConn; }

NavigatorBasic& PgsqlResult::navigator() { return _navigator; }

Oid PgsqlResult::tableOid()
{
   if (_pPGresult != nullptr) {
      return PQftable(_pPGresult, 0);
   }

   return -1;
}

// -------------------------------------------------------
// Override methods from base class : ResultCommon
// -------------------------------------------------------

TypeClass PgsqlResult::columnTypeClass(const int columnIndex) const
{
   throwIfColumnIndexInvalid(columnIndex);

   long nativeType = _columnInfoCollection[columnIndex].nativeType;
   TypeClass retVal = typeClassFromPgsqlType(nativeType);
   return retVal;
}

PgsqlResult::VariantType PgsqlResult::getVariantValue(const int columnIndex) const
{
   long row = _navigator.position();
   throwIfColumnIndexInvalid(columnIndex);
   throwIfRowIndexInvalid(row);
   throwIfPgResultInvalid();

   // Note: https://www.postgresql.org/docs/current/libpq-exec.html
   // PQgetvalue will return an empty string, not a null pointer, for a null field
   int isnull   = PQgetisnull(_pPGresult, row, columnIndex);
   char* rawVal = PQgetvalue(_pPGresult, row, columnIndex);

   if (isnull == 1)
      return std::monostate{};

   if (rawVal == nullptr)
      throw std::runtime_error("invalid result received from backend");

   try
   {
      DataType dataType = _columnInfoCollection[columnIndex].dataType;

      switch (dataType)
      {
         case DataType::smallint:
            return std::stoi(rawVal);
         case DataType::integer:
            return std::stol(rawVal);
         case DataType::bigint:
            return static_cast<int64_t>(std::stoll(rawVal));
         case DataType::float4:
            return std::stof(rawVal);
         case DataType::float8:
            return std::stod(rawVal);
         case DataType::boolean:
            return util::strToBool(rawVal);
         case DataType::varbinary:
         {
            // skip \x
            if (strlen(rawVal) > 3)
            {
               std::string hexString(rawVal + 2);
               return hexString;
            }
            else
               throw SqlException("invalid bytea data", "PgsqlResult");
         }
         case DataType::character:
         case DataType::varchar:
         case DataType::text:
         case DataType::numeric:
         case DataType::date:
         case DataType::time:
         case DataType::timestamp:
         default:
            return std::string(rawVal);
      }
   }
   catch (const SqlException &ex)
   {
      throw ex;
   }
   catch (const std::exception &ex)
   {
      throw tbs::SqlException(tbsfmt::format("getVariantValue, {}", ex.what()), "PgsqlResult");
   }
}

PgsqlResult::VariantType PgsqlResult::getVariantValue(const std::string& columnName) const
{
   return getVariantValue(columnNumber(columnName));
}

std::string PgsqlResult::getStringValue(const int columnIndex) const
{
   long row = _navigator.position();
   throwIfColumnIndexInvalid(columnIndex);
   throwIfRowIndexInvalid(row);
   throwIfPgResultInvalid();

   // Note: https://www.postgresql.org/docs/current/libpq-exec.html
   // PQgetvalue will return an empty string, not a null pointer, for a null field
   int isnull   = PQgetisnull(_pPGresult, row, columnIndex);
   char* rawVal = PQgetvalue(_pPGresult, row, columnIndex);

   if (isnull == 1)
      return sql::NULLSTR;

   if (rawVal == nullptr)
      throw tbs::SqlException("getStringValue, invalid result received from backend", "PgsqlResult");

   DataType dataType = _columnInfoCollection[columnIndex].dataType;
   if (dataType == DataType::varbinary)
   {
      // skip \x
      if (strlen(rawVal) > 3)
      {
         std::string hexString(rawVal + 2);
         return hexString;
      }
      else
         throw SqlException("invalid bytea data", "PgsqlResult");
   }
   else
      return std::string(rawVal);
}

std::string PgsqlResult::getStringValue(const std::string& columnName) const
{
   return getStringValue(columnNumber(columnName));
}

bool PgsqlResult::isNullField(const int columnIndex) const
{
   throwIfColumnIndexInvalid(columnIndex);
   throwIfRowIndexInvalid(_navigator.position());
   throwIfPgResultInvalid();

   return PQgetisnull(_pPGresult, _navigator.position(), columnIndex) != 0;
}

void PgsqlResult::throwIfPgResultInvalid() const
{
   if (_pPGresult == nullptr)
      throw SqlException("invalid pgresult object", "PgsqlResult");
}

void PgsqlResult::setupColumnProperties()
{
   if (_nColumns <= 0)
      return;

   _columnInfoCollection.reserve(_nColumns);
   for (int i = 0; i < _nColumns; i++)
   {
      _columnInfoCollection.emplace_back(ColumnInfo());
   }

   SqlApplyLogInternal applyLogRule(_pConn);

   try
   {
      for (int i = 0; i < _nColumns; i++)
      {
         // save column name
         _columnInfoCollection[i].name = std::string(PQfname(_pPGresult, i));

         // save column defined size
         long definedSize = 0;
         _columnInfoCollection[i].definedSize = definedSize;

         std::string sqlQry, sqlQry1, nativeTypeStr, fullTypeStr;
         Oid fieldTypeOid = PQftype(_pPGresult, i);
         int fieldTypeMod = PQfmod(_pPGresult, i);

         // save column native type as string
         sqlQry = tbsfmt::format("SELECT format_type(oid,NULL) as typname FROM pg_type WHERE oid = {}", fieldTypeOid);
         nativeTypeStr = _pConn->executeScalar(sqlQry);
         _columnInfoCollection[i].nativeTypeStr = nativeTypeStr;

         // save column native full type as string
         sqlQry1 = tbsfmt::format("SELECT format_type(oid,{}) as typname FROM pg_type WHERE oid = {}", fieldTypeMod, fieldTypeOid);
         fullTypeStr = _pConn->executeScalar(sqlQry1);
         _columnInfoCollection[i].nativeFullTypeStr = fullTypeStr;

         // save column native type
         _columnInfoCollection[i].nativeType = (long)fieldTypeOid;

         // save column data type : sql::DataType
         _columnInfoCollection[i].dataType = tbs::sql::pgsqlDataTypeToDataType(fieldTypeOid);
      }
   }
   catch (const TypeException & ex)
   {
      onNotifyError(_pConn->logId() + ex.what());
      throw tbs::SqlException(tbsfmt::format("setupColumnProperties, {}", ex.what()), "PgsqlResult");
   }
}

} // namespace sql
} // namespace tbs