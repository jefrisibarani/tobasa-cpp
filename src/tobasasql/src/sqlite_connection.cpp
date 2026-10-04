#include <tobasa/crypt.h>
#include <tobasa/bin_encode.h>
#include "tobasasql/exception.h"
#include "tobasasql/sql_dataset.h"
#include "tobasasql/sqlite_util.h"
#include "tobasasql/sqlite_command.h"
#include "tobasasql/sqlite_connection.h"

namespace tbs {
namespace sql {

SqliteConnection::SqliteConnection()
   : ConnectionCommon()
   , _pDatabase(nullptr)
   , _isEncrypted(false)
{
   notifierSource = "SqliteConnection";
}

SqliteConnection::SqliteConnection(SqliteConnection&& other) noexcept
   : ConnectionCommon(std::move(other))
   , _pDatabase(    other._pDatabase)
   , _databaseName( std::move(other._databaseName))
   , _isEncrypted(  other._isEncrypted)
{
   other._pDatabase   = nullptr;
   other._databaseName.clear();
   other._isEncrypted = false;
}

SqliteConnection& SqliteConnection::operator=(SqliteConnection&& other) noexcept
{
   if (this != &other)
   {
      disconnect();
      ConnectionCommon::operator=(std::move(other));

      _pDatabase    = other._pDatabase;
      _databaseName = std::move(other._databaseName);
      _isEncrypted  = other._isEncrypted;

      other._pDatabase   = nullptr;
      other._databaseName.clear();
      other._isEncrypted = false;
   }
   return *this;
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

   if (!paramOpenMemory && paramDatabase.empty())
   {
      onNotifyError(logId() + "SQLite Database file is not specified");
      return false;
   }

   int retCode = sqlite3_open_v2(paramDatabase.c_str(), (sqlite3**)&_pDatabase, openFlag, NULL);

   if (retCode == SQLITE_OK)
   {
      if (!paramOpenMemory && !paramPassword.empty() && keyDatabase(paramPassword) )
      {
         _connStatus = ConnectionStatus::ok;
         return true;
      }
      else
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
      _pDatabase   = nullptr;
      _isEncrypted = true;
      _connStatus  = ConnectionStatus::bad;

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

   SqliteCommand cmd(this);
   if (cmd.query(sql, parameters))
      return cmd.execute();
   else
      return -1;
}

std::string SqliteConnection::executeScalar(const std::string& sql, const SqlParameterCollection& parameters)
{
   if (status() != ConnectionStatus::ok)
      return "";

   SqliteCommand cmd(this);
   if (cmd.query(sql, parameters))
      return cmd.executeScalar();
   else 
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

bool SqliteConnection::startTransaction()
{
   return execute("BEGIN TRANSACTION") >= 0;
}

bool SqliteConnection::commitTransaction()
{
   return execute("COMMIT") >= 0;
}

bool SqliteConnection::rollbackTransaction()
{
   return execute("ROLLBACK") >= 0;
}

int64_t SqliteConnection::lastInsertRowid()
{
   if (status() != ConnectionStatus::ok)
      throw tbs::SqlException("Invalid connection status", "SqliteConnection");
   
   return sqlite3_last_insert_rowid(_pDatabase);
}

// -------------------------------------------------------
// Specific implementation functions
// -------------------------------------------------------

sqlite3* SqliteConnection::nativeConnection() const { return _pDatabase; }

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
   
   // Note: https://utelle.github.io/SQLite3MultipleCiphers/docs/ciphers/cipher_sqlcipher/
   // Number of iterations for key derivation
   sqlite3mc_config_cipher((sqlite3*)_pDatabase, "sqlcipher", "kdf_iter",              256000); 
   // Number of iterations for HMAC key derivation
   sqlite3mc_config_cipher((sqlite3*)_pDatabase, "sqlcipher", "fast_kdf_iter",         2);      
   // Flag whether a HMAC should be used
   sqlite3mc_config_cipher((sqlite3*)_pDatabase, "sqlcipher", "hmac_use",              1);      
   // Storage type for page number in HMAC: 0 = native, 1 = little endian, 2 = big endian
   sqlite3mc_config_cipher((sqlite3*)_pDatabase, "sqlcipher", "hmac_pgno",             1);      
   // Mask byte for HMAC salt
   sqlite3mc_config_cipher((sqlite3*)_pDatabase, "sqlcipher", "hmac_salt_mask",        0x3a);   
   // SQLCipher version to be used in legacy mode
   sqlite3mc_config_cipher((sqlite3*)_pDatabase, "sqlcipher", "legacy",                4);      
   // Page size to use in legacy mode, 0 = default SQLite page size
   sqlite3mc_config_cipher((sqlite3*)_pDatabase, "sqlcipher", "legacy_page_size",      4096);   
   // Hash algoritm for key derivation function 0 = SHA1, 1 = SHA256, 2 = SHA512
   sqlite3mc_config_cipher((sqlite3*)_pDatabase, "sqlcipher", "kdf_algorithm",         2);      
   // Hash algoritm for HMAC calculation 0 = SHA1, 1 = SHA256, 2 = SHA512
   sqlite3mc_config_cipher((sqlite3*)_pDatabase, "sqlcipher", "hmac_algorithm",        2);      
   // Size of plaintext database header must be a multiple of 16, i.e. 32
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
   if (_pDatabase == nullptr)
   {
      onNotifyError("lastBackendError, invalid connection object", "SqliteConnection");
      return "invalid connection object";
   }

   const char* err = sqlite3_errmsg(_pDatabase);
   if (err != nullptr && err[0] != '\0')
      return std::string(err);

   return "Unknown SQLite backend error";
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
   
   SqliteCommand cmd(this);
   if (! cmd.query(sql, parameters) )
      return false;

   auto dataSet = cmd.executeResult();
   if (dataSet == nullptr)
      return false;

   for (long i = 0; i < dataSet->totalRows(); i++)
   {
      auto variant = dataSet->data().at(0).at(0);
      objectNames.push_back(VariantHelper<>::toString(variant));
   }

   return true;
}


} // namespace sql
} // namespace tbs