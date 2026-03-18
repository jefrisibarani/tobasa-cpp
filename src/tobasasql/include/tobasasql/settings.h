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

/// Database context configuration options.
struct Database
{
   sql::BackendType dbDriver;
   std::string      connectionString;
   std::string      password;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Database, dbDriver, connectionString, password)

/// Connection option.
struct ConnectorOption
{
   Database    production;
   Database    development;
   std::string environment;      // development, production
   bool        logInternalSqlQuery;
   bool        logSqlQuery;
   std::string securitySalt;     // optional
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ConnectorOption, production, development, 
   environment, logInternalSqlQuery, logSqlQuery, securitySalt)

/** @}*/

} // namespace conf
} // namespace sql
} // namespace tbs