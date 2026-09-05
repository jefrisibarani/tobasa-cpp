#pragma once
#include <string>
#include <tobasa/json.h>
#include "tobasasql/common_types.h"

namespace tbs {
namespace sql {

/**
 * SQL Backend type serialixzation from json
 */
NLOHMANN_JSON_SERIALIZE_ENUM(BackendType, {
      {BackendType::pgsql,  "PGSQL"},
      {BackendType::sqlite, "SQLITE"},
      {BackendType::adodb,  "ADODB"},
      {BackendType::odbc,   "ODBC"},
      {BackendType::mysql,  "MYSQL"},
   })

namespace conf {

/** \addtogroup SQL
 * @{
 */

/// SQL connection options.
struct ConnectionOptions
{
   bool logSqlCommand;           // false
   bool connectTimeoutSeconds;   // 120 seconds
};

/// Database configuration options.
struct Database
{
   sql::BackendType dbDriver;
   /**
    * Connection string used to connect to the database, excluding the password.
    * The connector builds the final connection string by injecting the password
    * at runtime.
    */
   std::string      connectionString;
   /**
    * Password used to authenticate to the database.
    * If this value is encrypted, then securitySalt must be set to a valid salt
    * for decryption. Otherwise, the password is treated as plaintext.
    */
   std::string      password;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Database, dbDriver, connectionString, password)

/**
 * Connector configuration options.
 * This option provides two database configurations to choose from:
 * development and production.
 */
struct ConnectorOption
{
   Database    production;
   Database    development;
   std::string environment;      // development, production
   bool        logInternalSqlQuery;
   bool        logSqlQuery;
   /**
    * Optional salt used to decrypt an encrypted password.
    * When securitySalt is non-empty, the password in the corresponding Database
    * entry must be stored in encrypted form and decrypted with this salt.
    */
   std::string securitySalt;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ConnectorOption, production, development, 
   environment, logInternalSqlQuery, logSqlQuery, securitySalt)

/** @}*/

} // namespace conf
} // namespace sql
} // namespace tbs