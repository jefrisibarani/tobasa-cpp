#include "tobasasql/mysql_connection.h"
#include "tobasasql/mysql_command.h"

namespace tbs {
namespace sql {

MysqlConnection::MysqlConnection()
   : ConnectionCommon()
{
   _pMYcon        = nullptr;
   notifierSource = "MysqlConnection";
}

MysqlConnection::~MysqlConnection()
{
   disconnect();
}

std::string MysqlConnection::name()
{
   return "MySQL Connection";
}

bool MysqlConnection::connect(const std::string& connString)
{
   if (status() == ConnectionStatus::ok)
      return true;
   else
      disconnect();  // if we got broken connection, disconnect first, to reset _pMYcon

   _pMYcon = mysql_init(nullptr);

   if (_pMYcon == nullptr)
   {
      std::string errmsg = lastBackendError();
      onNotifyError(logId() + errmsg);
      disconnect();
      throw SqlException(errmsg, "MysqlConnection");
   }

   // "Database=sampledata;User=admin;Password=xxxxxxxx;Server=127.0.0.1;Port=3306"
   std::string parDatabase, parUser, parPassword, parHost, parPort;
   auto vectorParam = util::split(connString, ';');
   for (auto param : vectorParam)
   {
      if (util::startsWith(param, "Database"))
      {
         auto parLen = param.length();
         parDatabase = param.substr(9, parLen - (size_t)9);
      }
      if (util::startsWith(param, "User"))
      {
         auto parLen = param.length();
         parUser = param.substr(5, parLen - (size_t)5);
      }      
      if (util::startsWith(param, "Password"))
      {
         auto parLen = param.length();
         parPassword = param.substr(9, parLen - (size_t)9);
      }
      if (util::startsWith(param, "Server"))
      {
         auto parLen = param.length();
         parHost = param.substr(7, parLen - (size_t)7);
      }
      if (util::startsWith(param, "Port"))
      {
         auto parLen = param.length();
         parPort = param.substr(5, parLen - (size_t)5);
      }
   }

   int port;
   if (util::isNumber(parPort))
      port = std::stoi(parPort);
   else 
      throw SqlException("Invalid port in connecion string", "MysqlConnection");   

   MYSQL* handle = mysql_real_connect(
                     _pMYcon, parHost.c_str(), parUser.c_str(), parPassword.c_str(),
                     parDatabase.c_str(), port, nullptr, CLIENT_MULTI_STATEMENTS);

   if (handle == nullptr)
   {
      std::string errmsg = lastBackendError();
      onNotifyError(logId() + errmsg);
      disconnect();
      //mysql_close(_pMYcon);
      throw SqlException(errmsg, "MysqlConnection");
   }
   else {
      _connStatus = ConnectionStatus::ok;
      return true;
   }

   return false;
}

bool MysqlConnection::disconnect()
{
   if (_pMYcon != nullptr)
   {
      mysql_close(_pMYcon);
      _pMYcon = nullptr;
      _connStatus = ConnectionStatus::bad;
   }

   return true;
}

ConnectionStatus MysqlConnection::status()
{
   if (_pMYcon == nullptr)
   {
      _connStatus = ConnectionStatus::bad;
      return _connStatus;
   }

   if ( checkStatus() )
      return _connStatus;
   else 
   {
      _connStatus = ConnectionStatus::bad;
      throw tbs::SqlException("invalid connection object", "MysqlConnection");
   }
}

int MysqlConnection::execute(const std::string& sql, const MysqlParameterCollection& parameters)
{
   if (status() != ConnectionStatus::ok)
      return -1;

   if (logSqlQuery())
      onNotifyDebug(logId() + tbsfmt::format("execute: {}", sql));

   if (parameters.size() == 0)
   {
      if (mysql_real_query(_pMYcon, sql.c_str(), static_cast<unsigned long>(sql.length()) ) != 0)
         throwExceptionOnError();

      uint64_t affectedRows;
      //unsigned int numFields;
      MYSQL_RES* result = mysql_store_result(_pMYcon);
      if (result)
      {
         // we have data
         mysql_free_result(result);
         affectedRows = 0;
      }
      else
      {
         if(mysql_field_count(_pMYcon) == 0)
         {
            // no data returned, it was not a SELECT query
            affectedRows = mysql_affected_rows(_pMYcon);
         }
         else // mysql_store_result() should have returned data
            throwExceptionOnError();
      }

      //auto affectedRows = mysql_affected_rows(_pMYcon);
      if (affectedRows == UINT64_MAX)
         throwExceptionOnError();
      else if (affectedRows >= 0)
         return static_cast<int>(affectedRows);
   }
   else
   {
      MysqlCommand cmd(_pMYcon);
      cmd.init(sql, parameters);
      auto affectedRows = cmd.execute();
      // affectedRows should same as cmd.affectedRows();

      if (affectedRows >= 0)
        return static_cast<int>(affectedRows);
   }

   return -1;
}

std::string MysqlConnection::executeScalar(
   const std::string& sql,
   const MysqlParameterCollection& parameters)
{
   if (status() != ConnectionStatus::ok)
      throw tbs::SqlException("Invalid connection status", "MysqlConnection");

   if (logSqlQuery())
      onNotifyDebug(logId() + tbsfmt::format("execute: {}", sql));

   //bool success = false;
   if (parameters.size() == 0)
   {
      if (mysql_real_query(_pMYcon, sql.c_str(), static_cast<unsigned long>(sql.length()) ) != 0)
         throwExceptionOnError();

      //unsigned int numFields;
      MYSQL_RES *result = mysql_store_result(_pMYcon);
      if (result)
      {
         // we have data
         MYSQL_ROW row = mysql_fetch_row(result);
         if (row) 
         {
            std::string value = row[0];
            mysql_free_result(result);
            return value;
         }
         else
         {
            onNotifyTrace(logId() + "Scalar query returned fields but no row found");
            return {};
         }
      }
      else
      {
         if(mysql_field_count(_pMYcon) == 0)
         {
            // Scalar query returned no record
            onNotifyTrace(logId() + "Scalar query returned no record");
            return {};
         }   
         else // mysql_store_result() should have returned data
            throwExceptionOnError();
      }
   }
   else
   {
      MysqlCommand cmd(_pMYcon);
      cmd.init(sql, parameters);
      auto dbresult = cmd.executeResult();
      if (dbresult != nullptr)
      {
         if (dbresult->totalColumns == 0)
            onNotifyTrace(logId() + "Scalar query executed successfully with affected row: " + std::to_string(cmd.affectedRows()));
         
         if (dbresult->totalColumns > 0 && dbresult->totalRows == 0)
            onNotifyTrace(logId() + "Scalar query returned fields but no row found");
         
         if (dbresult->totalRows > 0)
         {
            auto variant = dbresult->data.at(0).at(0);
            return MysqlVariantHelper::toString(variant);
         }
      }
   }

   return "";
}

std::string MysqlConnection::versionString()
{
   if (status() != ConnectionStatus::ok)
      return "";
   
   SqlApplyLogInternal applyLogRule(this);
 
#if defined(TOBASA_SQL_USE_MARIADB_DLL)
   std::string mariaDBlib = "LibmariaDB";
#else   
   std::string mariaDBlib = "MariaDBClient";
#endif
   std::string libraryVersion;
   const char* libVer = mysql_get_client_info();
   if (libVer) {
      auto threadId = mysql_thread_id(_pMYcon);
      std::string threadSafe = mysql_thread_safe() ? "thread safe" : "not thread safe";
      libraryVersion = tbsfmt::format("{} v{}, on thread Id {} {}", mariaDBlib, std::string(libVer), threadId, threadSafe);
   }
   auto backendVersion = executeScalar("select VERSION()");
   backendVersion = " Server v" + backendVersion;
   return backendVersion + ", "  + libraryVersion;
}

std::string MysqlConnection::databaseName()
{
   if (status() != ConnectionStatus::ok)
      return "";
   
   SqlApplyLogInternal applyLogRule(this);
   return executeScalar("SELECT DATABASE()");
}

BackendType MysqlConnection::backendType() const { return BackendType::mysql; }

std::string MysqlConnection::dbmsName() { return name(); }

int64_t MysqlConnection::lastInsertRowid()
{
   if (status() != ConnectionStatus::ok)
      throw tbs::SqlException("Invalid connection status", "MysqlConnection");

   return (int64_t) mysql_insert_id(_pMYcon);
}

// -------------------------------------------------------
// Specific implementation functions
// -------------------------------------------------------

std::string MysqlConnection::lastBackendError()
{
   std::string errmsg;
   const char* myErr = 0;

   if (_pMYcon)
   {
      myErr = mysql_error(_pMYcon);
      if (myErr)
         errmsg = std::string(myErr);
   }

   return errmsg;
}

void MysqlConnection::throwExceptionOnError()
{
   std::string errmsg = lastBackendError();
   onNotifyError(logId() + errmsg );
   throw tbs::SqlException(errmsg, "MysqlConnection");
}

MYSQL* MysqlConnection::nativeConnection()
{
   return _pMYcon;
}

} // namespace sql
} // namespace tbs