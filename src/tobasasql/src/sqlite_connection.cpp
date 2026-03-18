#include <tobasa/crypt.h>
#include <tobasa/hextodec.h>
#include "tobasasql/exception.h"
#include "tobasasql/sql_dataset.h"
#include "tobasasql/sqlite_util.h"
#include "tobasasql/sqlite_connection.h"

namespace tbs {
namespace sql {

SqliteConnection::SqliteConnection()
   : ConnectionCommon()
{
   _pDatabase     = nullptr;
   _isEncrypted   = false;
   notifierSource = "SqliteConnection";
}

SqliteConnection::~SqliteConnection()
{
   disconnect();
   _pDatabase = nullptr;
}

std::string SqliteConnection::name()
{
   return "Sqlite Connection";
}

bool SqliteConnection::connect(const std::string& connString)
{
   if (status() == ConnectionStatus::ok)
      return true;

   std::string paramDatabase;
   std::string paramPassword;
   bool        paramOpenReadOnly  = false;
   bool        paramOpenReadWrite = false;
   bool        paramOpenCreate    = false;
   bool        paramOpenMemory    = false;

   auto vectorParam = util::split(connString, ';');
   for (auto param : vectorParam)
   {
      if (util::startsWith(param, "Database"))
      {
         auto parLen = param.length();
         paramDatabase = param.substr(9, parLen - (size_t)9);
      }
      if (util::startsWith(param, "Password"))
      {
         auto parLen = param.length();
         paramPassword = param.substr(9, parLen - (size_t)9);
      }
      if (util::startsWith(param, "OpenReadOnly"))
      {
         auto parLen = param.length();
         paramOpenReadOnly = ("True" == param.substr(13, parLen - (size_t)13));
      }
      if (util::startsWith(param, "OpenReadWrite"))
      {
         auto parLen = param.length();
         paramOpenReadWrite = ("True" == param.substr(14, parLen - (size_t)14));
      }
      if (util::startsWith(param, "OpenCreate"))
      {
         auto parLen = param.length();
         paramOpenCreate = ("True" == param.substr(11, parLen - (size_t)11));
      }
      if (util::startsWith(param, "OpenMemory"))
      {
         auto parLen = param.length();
         paramOpenMemory = ("True" == param.substr(11, parLen - (size_t)11));
      }
   }

   int openFlag = SQLITE_OPEN_READWRITE;

   if (paramOpenReadOnly) 
      openFlag = SQLITE_OPEN_READONLY;
   
   if (paramOpenReadWrite) 
      openFlag = SQLITE_OPEN_READWRITE;
   
   if (paramOpenCreate) 
      openFlag = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
   
   if (paramOpenMemory)
   {
      paramDatabase = ":memory:";
      openFlag = SQLITE_OPEN_READWRITE|SQLITE_OPEN_MEMORY;
   }

   int retCode = sqlite3_open_v2(paramDatabase.c_str(), (sqlite3**)&_pDatabase, openFlag, NULL);

   if (retCode == SQLITE_OK)
   {
      if (keyDatabase(paramPassword))
      {
         _connStatus = ConnectionStatus::ok;
         return true;
      }
   }
   else
   {
      onNotifyError(logId() + lastBackendError());
      disconnect();
   }

   return false;
}

bool SqliteConnection::disconnect()
{
   if (_pDatabase)
   {
      sqlite3_close(_pDatabase);
      _pDatabase = nullptr;
      _isEncrypted = true;
      _connStatus = ConnectionStatus::bad;

      return true;
   }

   return true;
}

ConnectionStatus SqliteConnection::status()
{
   if (_pDatabase == nullptr)
   {
      _connStatus = ConnectionStatus::bad;
      return _connStatus;
   }

   if ( checkStatus() )
      return _connStatus;
   else 
   {
      _connStatus = ConnectionStatus::bad;
      throw tbs::SqlException("invalid connection object", "SqliteConnection");
   }
}

int SqliteConnection::execute(const std::string& sql, const SqlParameterCollection& parameters)
{
   if (status() != ConnectionStatus::ok)
      return -1;

   if (logSqlQuery())
      onNotifyDebug(logId() + tbsfmt::format("execute: {}", sql));

   sqlite3_stmt* pStatement = createStatement(sql, parameters);
   int retCode              = sqlite3_step(pStatement);
   int affectedRows         = sqlite3_changes(_pDatabase);

   // INSERT, UPDATE, and DELETE
   if (retCode == SQLITE_DONE)
   {
      sqlite3_finalize(pStatement);

      // Only changes made directly by the INSERT, UPDATE or DELETE statement are considered affected rows
      if (affectedRows < 0)
         affectedRows = 0;

      if (logExecuteStatus())
         onNotifyTrace(logId() + tbsfmt::format("SQL command executed successfully, affectedRows: {}", affectedRows));
      
      return affectedRows;
   }
   // Query returns rows, ignore the result!
   else if (retCode == SQLITE_ROW)
   {
      sqlite3_finalize(pStatement);
      return 0;
   }
   else
   {
      auto errMessage = lastBackendError();
      onNotifyError(logId() + errMessage );
      sqlite3_finalize(pStatement);
      throw tbs::SqlException(tbsfmt::format("execute, {}", errMessage), "SqliteConnection");
   }

   return -1;
}

std::string SqliteConnection::executeScalar(const std::string& sql, const SqlParameterCollection& parameters)
{
   if (status() != ConnectionStatus::ok)
      throw tbs::SqlException("Invalid connection status", "SqliteConnection");

   if (logSqlQuery())
      onNotifyDebug(logId() + tbsfmt::format("executeScalar: {}", sql));

   sqlite3_stmt* pStatement = createStatement(sql, parameters);
   //int numColumns         = sqlite3_column_count(pStatement);
   int retCode              = sqlite3_step(pStatement);
   //int affectedRows       = sqlite3_changes(_pDatabase);

   if (retCode == SQLITE_ROW)
   {
      std::string result;
      if (nullptr == sqlite3_column_text(pStatement, 0))
         result = sql::NULLSTR;
      else
      {
         const unsigned char* val = sqlite3_column_text(pStatement,0);
         result = std::string((const char*)val);
      }

      sqlite3_finalize(pStatement);
      return result;
   }
   // INSERT, UPDATE, and DELETE
   else if (retCode == SQLITE_DONE)
   {
      sqlite3_finalize(pStatement);
      onNotifyInfo(logId() + "Scalar query returned no record");
      return "";
   }
   else
   {
      onNotifyError(logId() + lastBackendError());
      sqlite3_finalize(pStatement);
      throw tbs::SqlException(tbsfmt::format("execute, {}", lastBackendError()),"SqliteConnection");
   }

   return "";
}

std::string SqliteConnection::versionString() const
{
   const char* version = sqlite3_libversion();
   std::string retval = std::string(version);
   std::string val("SQLite Version ");
   return val + retval;
}

std::string SqliteConnection::databaseName()
{
   if (status() != ConnectionStatus::ok)
      return "";
   
   SqlApplyLogInternal applyLogRule(this);
   return executeScalar("select file from pragma_database_list where name='main'");
}

BackendType SqliteConnection::backendType() const { return BackendType::sqlite; }

std::string SqliteConnection::dbmsName() { return name(); }

int64_t SqliteConnection::lastInsertRowid()
{
   if (status() != ConnectionStatus::ok)
      throw tbs::SqlException("Invalid connection status", "SqliteConnection");
   
   return sqlite3_last_insert_rowid(_pDatabase);
}

// -------------------------------------------------------
// Specific implementation functions
// -------------------------------------------------------

sqlite3* SqliteConnection::nativeConn() const { return _pDatabase; }

bool SqliteConnection::keyDatabase(const std::string& key)
{
#ifdef TOBASA_SQL_USE_SQLITE3_MC
   int retCode = 0;
   const char* localKey = key.c_str();

   // use Sqlchiper v4
   retCode = sqlite3mc_config((sqlite3*)_pDatabase, "cipher", CODEC_TYPE_SQLCIPHER);
   if (retCode != CODEC_TYPE_SQLCIPHER)
   {
      onNotifyError(logId() + lastBackendError());
      return false;
   }

   sqlite3mc_config_cipher((sqlite3*)_pDatabase, "sqlcipher", "kdf_iter",              256000);
   sqlite3mc_config_cipher((sqlite3*)_pDatabase, "sqlcipher", "fast_kdf_iter",         2);
   sqlite3mc_config_cipher((sqlite3*)_pDatabase, "sqlcipher", "hmac_use",              1);
   sqlite3mc_config_cipher((sqlite3*)_pDatabase, "sqlcipher", "hmac_pgno",             1);
   sqlite3mc_config_cipher((sqlite3*)_pDatabase, "sqlcipher", "hmac_salt_mask",        0x3a);
   sqlite3mc_config_cipher((sqlite3*)_pDatabase, "sqlcipher", "legacy",                4);
   sqlite3mc_config_cipher((sqlite3*)_pDatabase, "sqlcipher", "legacy_page_size",      4096);
   sqlite3mc_config_cipher((sqlite3*)_pDatabase, "sqlcipher", "kdf_algorithm",         2);
   sqlite3mc_config_cipher((sqlite3*)_pDatabase, "sqlcipher", "hmac_algorithm",        2);
   sqlite3mc_config_cipher((sqlite3*)_pDatabase, "sqlcipher", "plaintext_header_size", 0);

   retCode = sqlite3_key((sqlite3*)_pDatabase, localKey, (int)key.length());
   if (retCode != SQLITE_OK)
   {
      onNotifyError(logId() + lastBackendError());
      return false;
   }

   // Note: https://utelle.github.io/SQLite3MultipleCiphers/docs/configuration/config_capi/
   // sqlite3_key return SQLITE_OK even if the provided key isnot correct.
   // test sql, to make sure key is valid
   char* szErrorMessage = NULL;
   retCode = sqlite3_exec(_pDatabase, "SELECT count(*) FROM sqlite_master", 0, 0, &szErrorMessage);
   if (retCode != SQLITE_OK)
   {
      onNotifyError( logId() + "Failed to open SQLite database");
      return false;
   }

   onNotifyDebug(logId() + std::string("Successfully opened SQLite database"));
   _isEncrypted = true;
   return _isEncrypted;

#else //TOBASA_SQL_USE_SQLITE3_MC
      return true;
#endif
}

bool SqliteConnection::rekeyDatabase(const std::string& newKey)
{
#ifdef TOBASA_SQL_USE_SQLITE3_MC

   const char* localNewKey = newKey.c_str();
   int retCode = sqlite3_rekey((sqlite3*)_pDatabase, localNewKey, (int)newKey.length());

   if (retCode != SQLITE_OK)
   {
      onNotifyError(logId() + lastBackendError());
      return false;
   }

   return true;
#else //TOBASA_SQL_USE_SQLITE3_MC
      return true;
#endif
}

std::string SqliteConnection::lastBackendError() const
{
   std::string errmsg;
   const char* sqliteErr = 0;

   if (_pDatabase)
   {
      sqliteErr = sqlite3_errmsg(_pDatabase);
      if (sqliteErr)
         errmsg = std::string(sqliteErr);
   }

   return errmsg;
}

sqlite3_stmt* SqliteConnection::createStatement(const std::string& sql, const SqlParameterCollection& parameters)
{
   sqlite3_stmt* pStatement = nullptr;

   try
   {
      const char* szTail = 0;
      const char* sqlBuffer = sql.c_str();
      int retCode = sqlite3_prepare_v2(_pDatabase, sqlBuffer, -1, &pStatement, &szTail);
      if (retCode != SQLITE_OK)
      {
         pStatement = nullptr;
         onNotifyError(logId() + lastBackendError());
         throw tbs::SqlException(tbsfmt::format("createStatement, {}", lastBackendError()), "SqliteConnection");
      }
      
      for (unsigned int i = 0; i < parameters.size(); i++)
      {
         int bindRc  = SQLITE_OK;
         auto& param = parameters.at(i);
         SqliteType parameterType = sqliteTypeFromDataType(param->type());

         if (std::holds_alternative<std::monostate>(param->value()))
         {
            // this is a param with monostate variant value. send it to backend as NULL
            bindRc = sqlite3_bind_null(pStatement, i + 1);
         }
         else 
         {
            std::string variantErrorMessage = "Invalid variant type for " + dataTypeToString(param->type());
            
            switch (parameterType)
            {
               case SqliteType::null:
                  bindRc = sqlite3_bind_null(pStatement, i + 1);
                  break;
               case SqliteType::integer:
               {
                  if (std::holds_alternative<bool>(param->value()))
                  {
                     bool bvalue = std::get<bool>(param->value());
                     auto paramValue = (int)((bvalue == true) ? 1 : 0);
                     bindRc = sqlite3_bind_int(pStatement, i + 1, paramValue);
                  }
                  else if (std::holds_alternative<int16_t>(param->value()))
                  {
                     auto paramValue = std::get<int16_t>(param->value());
                     bindRc = sqlite3_bind_int(pStatement, i + 1, paramValue);
                  }                  
                  else if (std::holds_alternative<int32_t>(param->value()))
                  {
                     auto paramValue = std::get<int32_t>(param->value());
                     bindRc = sqlite3_bind_int(pStatement, i + 1, paramValue);
                  }
                  else if (std::holds_alternative<int64_t>(param->value()))
                  {
                     auto paramValue = std::get<int64_t>(param->value());
                     bindRc = sqlite3_bind_int64(pStatement, i + 1, paramValue);
                  }
                  else
                  {
                     sqlite3_finalize(pStatement);
                     throw SqlException(variantErrorMessage, "SqliteConnection");
                  }
               }
                  break;
               case SqliteType::real:
               {
                  if (std::holds_alternative<float>(param->value()))
                  {
                     auto paramValue = std::get<float>(param->value());
                     bindRc = sqlite3_bind_double(pStatement, i + 1, paramValue);
                  }
                  else if (std::holds_alternative<double>(param->value()))
                  {
                     auto paramValue = std::get<double>(param->value());
                     bindRc = sqlite3_bind_double(pStatement, i + 1, paramValue);
                  }
                  else if (std::holds_alternative<std::string>(param->value()))
                  {
                     const char* paramValue = VariantHelper<>::value<std::string>(param->value()).c_str();
                     bindRc = sqlite3_bind_text(pStatement, i + 1, paramValue, -1, SQLITE_STATIC);
                  }
                  else 
                  {
                     sqlite3_finalize(pStatement);
                     throw SqlException(variantErrorMessage, "SqliteConnection");
                  }
               }
                  break;
               case SqliteType::text:
               {
                  const char* paramValue = VariantHelper<>::value<std::string>(param->value()).c_str();
                  bindRc = sqlite3_bind_text(pStatement, i + 1, paramValue, -1, SQLITE_STATIC);
               }
                  break;
               case SqliteType::blob:
               {
                  // BLOB. The value is a blob of data, stored exactly as it was input.
                  // store the data byte array
                  void* pBlob = *(param->valueBytePtr());
                  bindRc = sqlite3_bind_blob(pStatement, i + 1, (const void*)pBlob, param->size(), SQLITE_TRANSIENT);
               }
                  break;
               default:
               {
                  sqlite3_finalize(pStatement);
                  onNotifyError(logId() + "Invalid sqlite data type");
                  throw tbs::SqlException(tbsfmt::format("createStatement, invalid sqlite data type"), "SqliteConnection");
               }
                  break;
            }
         }

         if (bindRc != SQLITE_OK)
         {
            sqlite3_finalize(pStatement);
            onNotifyError(logId() + lastBackendError());
            throw tbs::SqlException(tbsfmt::format("createStatement, {}", lastBackendError()), "SqliteConnection");
         }
      }

      return pStatement;
   }
   catch (const std::bad_variant_access&)
   {
      sqlite3_finalize(pStatement);
      throw tbs::SqlException("createStatement, bad variant access", "SqliteConnection");
   }
   catch (const tbs::VariantException& ex)
   {
      sqlite3_finalize(pStatement);
      throw tbs::SqlException(tbsfmt::format("createStatement, {}", ex.what()),"SqliteConnection");
   }
   catch (const tbs::TypeException& ex)
   {
      sqlite3_finalize(pStatement);
      throw tbs::SqlException(tbsfmt::format("createStatement, {}", ex.what()),"SqliteConnection");
   }

   return nullptr;
}

std::shared_ptr<DataSet<DefaultVariantType>> SqliteConnection::executeResult(
   const std::string& sql,
   const SqlParameterCollection& parameters)
{
   if (status() != ConnectionStatus::ok)
      return nullptr;

   if (logSqlQuery())
      onNotifyDebug(logId() + tbsfmt::format("executeScalar: {}", sql));

   sqlite3_stmt* pStatement = createStatement(sql, parameters);
   //int affectedRows       = sqlite3_changes(_pDatabase);
   int numColumns           = sqlite3_column_count(pStatement);
   int retCode              = 0;
   int resultsRetrieved     = 0;
   auto dbresult            = std::make_shared<DataSet<DefaultVariantType>>();
   dbresult->totalColumns   = numColumns;

   while (true)
   {
      retCode = sqlite3_step(pStatement);

      if (retCode == SQLITE_DONE)
         break;
      else if (retCode == SQLITE_ROW)
      {
         auto& newRow = dbresult->addRow();

         int i = 0;
         for (i = 0; i < numColumns; i++)
         {
            if (NULL == sqlite3_column_text(pStatement, i) )
               newRow.push_back(sql::NULLSTR);
            else
            {
               std::string value = (const char*)sqlite3_column_text(pStatement, i);
               newRow.push_back(value);;
            }
         }
         resultsRetrieved++;
      }
      else
      {
         auto errorMessage = lastBackendError();
         onNotifyError(logId() + errorMessage);
         sqlite3_finalize(pStatement);
         throw tbs::SqlException(tbsfmt::format("executeResult, {}", errorMessage), "SqliteConnection");
      }
   }

   dbresult->totalRows = resultsRetrieved;

   if (retCode == SQLITE_DONE)
   {
      sqlite3_finalize(pStatement);
      return dbresult;
   }

   return nullptr;
}

bool SqliteConnection::tableOrViewExists(const std::string& tableName, bool checkTable)
{
   std::string tableType = checkTable ? "table" : "view";
   onNotifyTrace(logId() + tbsfmt::format("getTablesOrViews: type is: {}", tableType));

   SqlApplyLogInternal applyLogRule(this);

   std::string sql("SELECT name FROM sqlite_master WHERE type=? AND name NOT LIKE 'sqlite_%' AND name=? ORDER BY name");
   SqlParameterCollection parameters;
   auto paramType = std::make_shared<SqlParameter>("type", sql::DataType::varchar, tableType);
   auto paramName = std::make_shared<SqlParameter>("name", sql::DataType::varchar, tableName);
   parameters.push_back(paramType);
   parameters.push_back(paramName);
   std::string tbl = executeScalar(sql,parameters);

   return (tableName == tbl );
}

bool SqliteConnection::getTablesOrViews(std::vector<std::string>& objectNames, bool getTables)
{
   std::string tableType = getTables ? "table" : "view";
   onNotifyTrace(logId() + tbsfmt::format("getTablesOrViews: type is: {}", tableType));

   SqlApplyLogInternal applyLogRule(this);

   std::string sql("SELECT name FROM sqlite_master WHERE type=? AND name NOT LIKE 'sqlite_%' ORDER BY name");
   SqlParameterCollection parameters;
   auto paramType = std::make_shared<SqlParameter>("type", sql::DataType::varchar, tableType);
   parameters.push_back(paramType);
   auto dbresult = executeResult(sql,parameters);
   
   if (dbresult == nullptr) {
      return false;
   }

   for (long i = 0; i < dbresult->totalRows; i++)
   {
      auto variant = dbresult->data.at(0).at(0);
      objectNames.push_back(VariantHelper<>::toString(variant));
   }

   return true;
}


} // namespace sql
} // namespace tbs