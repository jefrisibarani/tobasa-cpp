#include <tobasa/variant.h>
#include <tobasa/variant_helper.h>
#include "tobasasql/sql_dataset.h"
#include "tobasasql/sqlite_util.h"
#include "tobasasql/sql_util.h"
#include "tobasasql/sqlite_result.h"
#include "tobasasql/sqlite_connection.h"
#include "tobasasql/sqlite_command.h"

namespace tbs {
namespace sql {

SqliteCommand::SqliteCommand(SqliteConnection* conn)
   : _pConn(conn)
   , _pSqliteDb(nullptr)
   , _pStatement(nullptr)
   , _affectedRows(-1)
{
   if (_pConn)
      _pSqliteDb = _pConn->nativeConnection();
   
   notifierSource = "SqliteCommand";
}

SqliteCommand::~SqliteCommand()
{
   close();
}

bool SqliteCommand::query(const std::string& sql, const SqlParameterCollection& parameters)
{
   if (!prepare(sql))
      return false;

   return bind(parameters);
}

bool SqliteCommand::prepare(const std::string& sql)
{
   if (_pSqliteDb == nullptr)
      throw SqlException("Invalid sqlite db object", "SqliteCommand");

   close();

   _sql = sql;

   const char* szTail = 0;
   const char* sqlBuffer = sql.c_str();
   int rc = sqlite3_prepare_v2(_pSqliteDb, sqlBuffer, -1, &_pStatement, &szTail);
   if (rc != SQLITE_OK)
   {
      _pStatement = nullptr;
      std::string errMsg = lastBackendError();
      onNotifyError(_pConn->logId() + errMsg);
      throw SqlException(tbsfmt::format("prepare, {}", errMsg ), "SqliteCommand");
   }

   return true;
}

bool SqliteCommand::bind(const SqlParameterCollection& parameters)
{
   if (_pSqliteDb == nullptr)
      throw SqlException("Invalid sqlite db object", "SqliteCommand");

   if (_pStatement == nullptr)
      throw SqlException("Invalid statement object", "SqliteCommand");

   onNotifyTrace(_pConn->logId() + "SqliteCommand, Binding paramaters" );

   sqlite3_clear_bindings(_pStatement);

   try
   {
      for (unsigned int i = 0; i < parameters.size(); i++)
      {
         int rc  = SQLITE_OK;
         const auto& param = parameters.at(i);
         const int index = static_cast<int>(i + 1);
         SqliteType parameterType = sqliteTypeFromDataType(param->type());

         if (std::holds_alternative<std::monostate>(param->value()))
         {
            // this is a param with monostate variant value. send it to backend as NULL
            rc = sqlite3_bind_null(_pStatement, index);
         }
         else 
         {
            std::string variantErrorMessage = "Invalid variant type for " + dataTypeToString(param->type());
            
            switch (parameterType)
            {
               case SqliteType::null:
               {
                  rc = sqlite3_bind_null(_pStatement, index);
                  break;
               }
               case SqliteType::integer:
               {
                  if (std::holds_alternative<bool>(param->value()))
                  {
                     bool bvalue = std::get<bool>(param->value());
                     auto paramValue = (int)((bvalue == true) ? 1 : 0);
                     rc = sqlite3_bind_int(_pStatement, index, paramValue);
                  }
                  else if (std::holds_alternative<int16_t>(param->value()))
                  {
                     auto paramValue = std::get<int16_t>(param->value());
                     rc = sqlite3_bind_int(_pStatement, index, paramValue);
                  }
                  else if (std::holds_alternative<int32_t>(param->value()))
                  {
                     auto paramValue = std::get<int32_t>(param->value());
                     rc = sqlite3_bind_int(_pStatement, index, paramValue);
                  }
                  else if (std::holds_alternative<int64_t>(param->value()))
                  {
                     auto paramValue = std::get<int64_t>(param->value());
                     rc = sqlite3_bind_int64(_pStatement, index, paramValue);
                  }
                  else {
                     throw SqlException(variantErrorMessage, "SqliteCommand");
                  }

                  break;
               }
               case SqliteType::real:
               {
                  if (std::holds_alternative<float>(param->value()))
                  {
                     auto paramValue = std::get<float>(param->value());
                     rc = sqlite3_bind_double(_pStatement, index, paramValue);
                  }
                  else if (std::holds_alternative<double>(param->value()))
                  {
                     auto paramValue = std::get<double>(param->value());
                     rc = sqlite3_bind_double(_pStatement, index, paramValue);
                  }
                  else if (std::holds_alternative<std::string>(param->value()))
                  {
                     const char* paramValue = SqliteCommand::VariantHelper::value<std::string>(param->value()).c_str();
                     rc = sqlite3_bind_text(_pStatement, index, paramValue, -1, SQLITE_STATIC);
                  }
                  else {
                     throw SqlException(variantErrorMessage, "SqliteCommand");
                  }

                  break;
               }
               case SqliteType::text:
               {
                  if (std::holds_alternative<double>(param->value()))
                  {
                     auto paramValue = std::get<double>(param->value());
                     rc = sqlite3_bind_double(_pStatement, index, paramValue);
                  }
                  else
                  {
                     const char* paramValue = SqliteCommand::VariantHelper::value<std::string>(param->value()).c_str();
                     rc = sqlite3_bind_text(_pStatement, index, paramValue, -1, SQLITE_STATIC);
                  }

                  break;
               }
               case SqliteType::blob:
               {
                  // BLOB. The value is a blob of data, stored exactly as it was input.
                  // store the data byte array
                  void* pBlob = *(param->valueBinaryPtr());
                  rc = sqlite3_bind_blob(_pStatement, index, (const void*)pBlob, static_cast<int>(param->size()), SQLITE_TRANSIENT);

                  break;
               }
               default:
               {
                  onNotifyError(_pConn->logId() + "Invalid sqlite data type");
                  throw SqlException(tbsfmt::format("bind, invalid sqlite data type"), "SqliteCommand");
               }
               break;
            }
         }

         if (rc != SQLITE_OK)
         {
            onNotifyError(_pConn->logId() + lastBackendError());
            throw SqlException(tbsfmt::format("bind, {}", lastBackendError()), "SqliteCommand");
         }
      }
   }
   catch (const std::bad_variant_access&)
   {
      throw SqlException("bind, bad variant access", "SqliteCommand");
   }
   catch (const VariantException& ex)
   {
      throw SqlException(tbsfmt::format("bind, {}", ex.what()),"SqliteCommand");
   }
   catch (const TypeException& ex)
   {
      throw SqlException(tbsfmt::format("bind, {}", ex.what()),"SqliteCommand");
   }

   return true;
}

void SqliteCommand::reset()
{
   if (_pSqliteDb == nullptr)
      throw SqlException("Invalid sqlite db object", "SqliteCommand");

   if (_pStatement == nullptr)
      throw SqlException("Invalid statement object", "SqliteCommand");

   sqlite3_reset(_pStatement);
   sqlite3_clear_bindings(_pStatement);
   _affectedRows = -1;
}

void SqliteCommand::close()
{
   if (_pStatement != nullptr)
   {
      sqlite3_finalize(_pStatement);
      _pStatement = nullptr;
   }

   _sql.clear();
   _affectedRows = -1;
}

int SqliteCommand::execute()
{
   if (_pSqliteDb == nullptr)
      throw SqlException("Invalid sqlite db object", "SqliteCommand");

   if (_pStatement == nullptr)
      throw tbs::SqlException("Invalid statement object", "SqliteCommand");

   if (_pConn->logSqlQuery())
      onNotifyDebug(_pConn->logId() + tbsfmt::format("execute: {}", _sql));

   int rc = sqlite3_step(_pStatement);

   if (rc == SQLITE_DONE || rc == SQLITE_ROW)
   {
      // INSERT, UPDATE, and DELETE
      if (rc == SQLITE_DONE)
      {
         // Only changes made directly by the INSERT, UPDATE or DELETE statement are considered affected rows
         _affectedRows = sqlite3_changes(_pSqliteDb);
      }
      // Query returns rows, ignore the result!
      if (rc == SQLITE_ROW) {
         _affectedRows = 0;
      }

      if (_pConn->logExecuteStatus())
         onNotifyDebug(_pConn->logId() + tbsfmt::format("SQL command executed successfully, affectedRows: {}", _affectedRows));

      return _affectedRows;
   }
   else
   {
      auto errMessage = statementError();
      onNotifyError(_pConn->logId() + errMessage );
      throw SqlException(tbsfmt::format("execute, {}", errMessage), "SqliteCommand");
   }

   return -1; // should not reach here
}

std::string SqliteCommand::executeScalar()
{
   if (_pSqliteDb == nullptr)
      throw SqlException("Invalid sqlite db object", "SqliteCommand");

   if (_pStatement == nullptr)
      throw tbs::SqlException("Invalid statement object", "SqliteCommand");

   if (_pConn->logSqlQuery())
      onNotifyDebug(_pConn->logId() + tbsfmt::format("executeScalar: {}", _sql));

   int rc = sqlite3_step(_pStatement);

   // Query returns rows, get the result!
   if (rc == SQLITE_ROW) 
   {
      _affectedRows = 0;

      std::string result;
      if (nullptr == sqlite3_column_text(_pStatement, 0))
      {
         if (_pConn->logExecuteStatus())
            onNotifyDebug(_pConn->logId() + "Scalar query returned SQL NULL");

         result = sql::NULLSTR;
      }
      else
      {
         const unsigned char* val = sqlite3_column_text(_pStatement, 0);
         result = std::string((const char*)val);
      }

      if (_pConn->logExecuteStatus())
         onNotifyDebug(_pConn->logId() + tbsfmt::format("Scalar query executed successfully"));

      return result;
   }
   // INSERT, UPDATE, and DELETE
   else if (rc == SQLITE_DONE)
   {
      // Only changes made directly by the INSERT, UPDATE or DELETE statement are considered affected rows
      _affectedRows = sqlite3_changes(_pSqliteDb);

      if (_pConn->logExecuteStatus())
         onNotifyDebug(_pConn->logId() + tbsfmt::format("Scalar query returned no record"));

      return {};
   }
   else
   {
      auto errmsg = statementError();
      onNotifyError(_pConn->logId() + errmsg );
      throw SqlException(tbsfmt::format("executeScalar, {}", errmsg), "SqliteCommand");
   }

   return {}; // should not reach here
}

SqliteCommand::DataSetPtr SqliteCommand::executeResult()
{
   if (_pSqliteDb == nullptr)
      throw SqlException("Invalid sqlite db object", "SqliteCommand");
      
   if (_pStatement == nullptr)
      throw tbs::SqlException("Invalid statement object", "SqliteCommand");

   if (_pConn->logSqlQuery())
      onNotifyDebug(_pConn->logId() + tbsfmt::format("executeResult: {}", _sql));

   int nRows             = 0;
   const int nCols       = sqlite3_column_count(_pStatement);
   _affectedRows         = 0;

   auto dataSet          = std::make_shared<DataSet<VariantType>>();
   dataSet->totalColumns(nCols);
   
   int rc = 0;
   while (true)
   {
      rc = sqlite3_step(_pStatement);
      _affectedRows += sqlite3_changes(_pConn->nativeConnection());

      if (rc == SQLITE_DONE) {
         break;
      }
      else if (rc == SQLITE_ROW)
      {
         VectorVariant cols;

         if (nCols > 0) {
            cols.reserve(nCols);
         }

         for (int i = 0; i < nCols; i++)
         {
            int colType = sqlite3_column_type(_pStatement, i);

            auto declaredType = sqliteDeclaredTypeToDataType(_pStatement, i);

            SqliteType sqliteType = (SqliteType)colType;

            switch (sqliteType)
            {
               case SqliteType::null:     // SQLITE_NULL
               {
                  cols.emplace_back( std::monostate{} );
                  break;
               }
               case SqliteType::integer:  // SQLITE_INTEGER
               {
                  int64_t value = static_cast<int64_t>(sqlite3_column_int64(_pStatement, i));
                  if (declaredType == DataType::boolean)
                  {
                     bool boolValue = value > 0;
                     cols.emplace_back(boolValue);
                  }
                  else {
                     cols.emplace_back(value);
                  }
                  break;
               }
               case SqliteType::real:     // SQLITE_FLOAT
               {
                  std::string valueStr = (const char*)sqlite3_column_text(_pStatement, i);

                  if (declaredType == DataType::numeric)
                  {
                     cols.emplace_back(valueStr);
                  }
                  else if (declaredType == DataType::float4)
                  {
                     float value = std::stof(valueStr);
                     cols.emplace_back(value);
                  }
                  else /*(declaredType == DataType::float8)*/
                  {
                     double value = sqlite3_column_double(_pStatement, i);
                     cols.emplace_back(value);
                  }
                  break;
               }
               case SqliteType::text:     // SQLITE_TEXT
               {
                  std::string value = (const char*)sqlite3_column_text(_pStatement, i);
                  if (declaredType == DataType::boolean)
                  {
                     bool boolValue = util::strToBool(value);
                     cols.emplace_back(boolValue);
                  }
                  else {
                     cols.emplace_back(value);
                  }
                  break;
               }
               case SqliteType::blob:     // SQLITE_BLOB
               {
                  // BLOB. The value is a blob of data, stored exactly as it was input.
                  // SQLite store blob as binary data, so we need to convert first to hex string
                  int blobSize = sqlite3_column_bytes(_pStatement, i);
                  tbs::byte_t* raw = (tbs::byte_t*)sqlite3_column_blob(_pStatement, i);
                  std::string resStr;
                  for (int i = 0; i < blobSize; ++i)
                  {
                     tbs::byte_t b = raw[i];
                     resStr += conv::decToHex(b);
                  }
                  cols.emplace_back(resStr);
                  break;
               }
               default:
               {
                  std::string value = (const char*)sqlite3_column_text(_pStatement, i);
                  cols.emplace_back(value);
                  break;
               }
            }
         }

         dataSet->data().emplace_back(std::move(cols));
         nRows++;
      }
      else
      {
         onNotifyError(_pConn->logId() + lastBackendError());
         throw tbs::SqlException(tbsfmt::format("executeResult, {}", statementError()), "SqliteCommand");
      }
   }


   if (rc == SQLITE_DONE)
   {
      if (_pConn->logExecuteStatus())
         onNotifyDebug(_pConn->logId() + tbsfmt::format("SQL command executed successfully, row: {}, columns: {} ", nRows, nCols));
   }

   return std::move(dataSet);
}

SqliteResult SqliteCommand::executeSqlResult()
{
   SqliteResult result;
   result.connection(_pConn);
   result.runPreparedQuery(*this);

   return std::move(result);
}

std::string SqliteCommand::lastBackendError()
{
   if (_pSqliteDb == nullptr)
   {
      onNotifyError("lastBackendError, invalid connection object", "SqliteCommand");
      return "invalid connection object";
   }

   const char* err = sqlite3_errmsg(_pSqliteDb);
   if (err != nullptr && err[0] != '\0')
      return std::string(err);

   return "Unknown SQLite backend error";
}

std::string SqliteCommand::statementError()
{
   if (_pStatement == nullptr)
      return lastBackendError();

   const char* err = sqlite3_errmsg(_pSqliteDb);
   if (err != nullptr && err[0] != '\0')
      return std::string(err);

   return "Unknown SQLite statement error";
}

int SqliteCommand::affectedRows() 
{ 
   return _affectedRows; 
}

// --------------------------------------------------------------------------

} // namespace sql
} // namespace tbs