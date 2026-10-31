#include "tobasasql/mysql_connection.h"
#include "tobasasql/mysql_command.h"
#include "tobasasql/sql_util.h"
#include <tobasa/bin_encode.h>

namespace tbs {
namespace sql {

MysqlConnection::MysqlConnection()
   : ConnectionCommon()
   , _pMYcon(nullptr)
{
   notifierSource = "MysqlConnection";
}

MysqlConnection::MysqlConnection(MysqlConnection&& other) noexcept
   : ConnectionCommon(std::move(other))
   , _pMYcon(other._pMYcon)
{
   other._pMYcon = nullptr;
}

MysqlConnection& MysqlConnection::operator=(MysqlConnection&& other) noexcept
{
   if (this != &other)
   {
      disconnect();
      ConnectionCommon::operator=(std::move(other));
      _pMYcon = other._pMYcon;
      other._pMYcon = nullptr;
   }
   return *this;
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
   auto checkResult = [this] (int result, const std::string& errMessage="") {
      if (result != 0)
      {
         std::string errmsg = lastBackendError();
         if (errmsg.empty())
            errmsg = errMessage;

         onNotifyError(logId() + errmsg);
         disconnect();
         throw SqlException(errmsg, "MysqlConnection");
      }
   };

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

   onNotifyInfo(tbsfmt::format("{}MariaDB C Connector version {}", logId(), mysql_get_client_info() ));

   int rc = 0;

   // Plugin directory
   std::string pluginDir = path::executableDir() + path::SEPARATOR + "mariadb_plugin";
   if (path::exists(pluginDir))
   {
      rc = mysql_optionsv(_pMYcon, MYSQL_PLUGIN_DIR, const_cast<char*>(pluginDir.c_str()));
      checkResult(rc, "Failed to configure MariaDB MYSQL_PLUGIN_DIR");
   }

   // Prevent loading my.ini / my.cnf.
   rc = mysql_optionsv(_pMYcon, MYSQL_READ_DEFAULT_FILE, nullptr);
   checkResult(rc, "Failed to configure MariaDB MYSQL_READ_DEFAULT_FILE");
   
   rc = mysql_optionsv(_pMYcon, MYSQL_READ_DEFAULT_GROUP, nullptr);
   checkResult(rc, "Failed to configure MariaDB MYSQL_READ_DEFAULT_GROUP");


   // TODO_JEFRI: add TLS connection string parameters
   // SSLENFORCE=true;
   // SSLVERIFY=true;
   // SSLCA=C:/certs/ca.pem;
   // SSLCERT=;
   // SSLKEY=;

   // "Database=sampledata;User=admin;Password=xxxxxxxx;Server=127.0.0.1;Port=3306"
   std::string parDatabase, parUser, parPassword, parHost, parPort;
   std::string parSslEnforce, parSslVerify, parSslCa, parSslCert, parSSlKey;
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

      if (util::startsWith(param, "SSLVERIFY"))
      {
         auto parLen = param.length();
         parSslVerify = param.substr(10, parLen - (size_t)10);
      }
      if (util::startsWith(param, "SSLCA"))
      {
         auto parLen = param.length();
         parSslCa = param.substr(6, parLen - (size_t)6);
      }
      if (util::startsWith(param, "SSLCERT"))
      {
         auto parLen = param.length();
         parSslCert = param.substr(8, parLen - (size_t)8);
      }
      if (util::startsWith(param, "SSLKEY"))
      {
         auto parLen = param.length();
         parSSlKey = param.substr(7, parLen - (size_t)7);
      }
   }

   int port;
   if (util::isNumber(parPort))
      port = std::stoi(parPort);
   else 
      throw SqlException("Invalid port in connection string", "MysqlConnection");

   my_bool sslEnforce = (parSslEnforce=="true") ? 1 : 0;
   rc = mysql_optionsv(_pMYcon, MYSQL_OPT_SSL_ENFORCE, (void*)&sslEnforce);
   checkResult(rc, "Failed to configure MariaDB MYSQL_OPT_SSL_ENFORCE");

   my_bool sslVerify = (parSslVerify=="true") ? 1 : 0;
   rc = mysql_optionsv(_pMYcon, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, (void*)&sslVerify);
   checkResult(rc, "Failed to configure MariaDB MYSQL_OPT_SSL_VERIFY_SERVER_CERT");

   if (rc==0 && sslVerify==1 && sslEnforce==1)
   {
      if (!parSslCa.empty()) 
      {
         rc = mysql_optionsv(_pMYcon, MYSQL_OPT_SSL_CA,   parSslCa.c_str());
         checkResult(rc, "Failed to configure MariaDB MYSQL_OPT_SSL_CA");
      }

      if (!parSslCert.empty())
      {
         rc = mysql_optionsv(_pMYcon, MYSQL_OPT_SSL_CERT, parSslCert.c_str());
         checkResult(rc, "Failed to configure MariaDB MYSQL_OPT_SSL_CERT");
      }

      if (!parSSlKey.empty())
      {
         rc = mysql_optionsv(_pMYcon, MYSQL_OPT_SSL_KEY,  parSSlKey.c_str());
         checkResult(rc, "Failed to configure MariaDB MYSQL_OPT_SSL_KEY");
      }
   }


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
   else 
   {
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

   if (parameters.size() > 0)
   {
      MysqlCommand cmd(this);
      if (!cmd.query(sql, parameters))
         return -1;

      return cmd.execute();
   }
   else
   {
      if (logSqlQuery())
         onNotifyDebug(logId() + tbsfmt::format("execute: {}", sql));

      if (mysql_real_query(_pMYcon, sql.c_str(), static_cast<unsigned long>(sql.length()) ) != 0)
         throwExceptionOnError();

      uint64_t affectedRows;
      MYSQL_RES* result = mysql_store_result(_pMYcon);
      if (result)
      {
         // we have data
         mysql_free_result(result);
         affectedRows = 0;
      }
      else
      {
         if (mysql_field_count(_pMYcon) == 0)
         {
            // no data returned, it was not a SELECT query
            affectedRows = mysql_affected_rows(_pMYcon);
         }
         else // mysql_store_result() should have returned data
            throwExceptionOnError();
      }

      if (affectedRows == UINT64_MAX)
         throwExceptionOnError();
      else if (affectedRows >= 0)
      {
         if (logExecuteStatus())
            onNotifyDebug(logId() + tbsfmt::format("SQL command executed successfully, affectedRows: {}", affectedRows));

         return static_cast<int>(affectedRows);
      }
   }

   return -1;
}

std::string MysqlConnection::executeScalar(const std::string& sql, const MysqlParameterCollection& parameters)
{
   if (status() != ConnectionStatus::ok)
      return "";

   if (parameters.size() > 0)
   {
      MysqlCommand cmd(this);
      if (!cmd.query(sql, parameters))
         return "";

      return cmd.executeScalar();
   }
   else
   {
      if (logSqlQuery())
         onNotifyDebug(logId() + tbsfmt::format("execute: {}", sql));

      if (mysql_real_query(_pMYcon, sql.c_str(), static_cast<unsigned long>(sql.length()) ) != 0)
         throwExceptionOnError();

      MYSQL_RES *result = mysql_store_result(_pMYcon);
      if (result)
      {
         MYSQL_ROW row = mysql_fetch_row(result);
         if (row == nullptr)
         {
            mysql_free_result(result);

            if (logExecuteStatus())
               onNotifyDebug(logId() + "Scalar query returned no row");

            return "";
         }
         // check for SQL NULL
         if (row[0] == nullptr)
         {
            mysql_free_result(result);

            if (logExecuteStatus())
               onNotifyDebug(logId() + "Scalar query returned SQL NULL");

            return sql::NULLSTR;  // "null"
         }

         MYSQL_FIELD* fields = mysql_fetch_fields(result);
         const enum_field_types columnType = fields[0].type;
         const unsigned int columnCount = mysql_num_fields(result);
         unsigned long* lengths = mysql_fetch_lengths(result);

         std::string value;
         if (lengths && lengths[0] > 0)
         {
            if (columnType == MYSQL_TYPE_BIT)
            {
               // MySQL BIT is returned as raw bytes; convert each byte to an 8-bit textual bit string.
               value = conv::binaryBytesToString((byte_t*)row[0],lengths[0]);
            }
            else
               value.assign(row[0], row[0] + lengths[0]);
         }
         else
         {
            mysql_free_result(result);

            if (logExecuteStatus())
               onNotifyDebug(logId() + "Scalar query returned invalid or empty data length");

            return "";
         }

         mysql_free_result(result);
         return value;
      }
      else
      {
         if (mysql_field_count(_pMYcon) == 0)
         {
            // Scalar query returned no record
            onNotifyDebug(logId() + "Scalar query returned no record");
            return "";
         }   
         else // mysql_store_result() should have returned data
            throwExceptionOnError();
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
   std::string mariaDBlib = "LibMariaDB";
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

bool MysqlConnection::startTransaction()
{
   return execute("START TRANSACTION") >= 0;
}

bool MysqlConnection::commitTransaction()
{
   return execute("COMMIT") >= 0;
}

bool MysqlConnection::rollbackTransaction()
{
   return execute("ROLLBACK") >= 0;
}

bool MysqlConnection::isTranscationActive()
{
   if (status() != ConnectionStatus::ok || _pMYcon == nullptr)
      return false;

   return ( _pMYcon->server_status & SERVER_STATUS_IN_TRANS ) != 0;
}

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
   if (_pMYcon == nullptr)
   {
      onNotifyError("lastBackendError, invalid connection object", "MysqlConnection");
      return "invalid connection object";
   }

   const char* err = mysql_error(_pMYcon);
   if (err != nullptr && err[0] != '\0')
      return std::string(err);

   return "Unknown MySQL backend error";
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