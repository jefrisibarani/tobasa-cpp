#include <tobasa/config.h>
#include <tobasalis/lis/settings.h>
#include <tobasasql/common_types.h>
#include "lis_util.h"

namespace tbs {
namespace lis {

sql::conf::ConnectorOption getDbConnectorOption()
{
   auto opt = Config::getOption<conf::Engine>("lisEngine");
   sql::BackendType dbDriverType = sql::backendTypeFromString(opt.database.dbDriver);
   
   sql::conf::ConnectorOption connOpt;
   connOpt.logInternalSqlQuery   = opt.database.logInternalSqlQuery;
   connOpt.logSqlQuery           = opt.database.logSqlQuery;
   
   // securitySalt must be initialized
   connOpt.securitySalt          = Config::getOption<std::string>("securitySalt");
   connOpt.environment           = "production";
   connOpt.production.dbDriver   = dbDriverType;
   connOpt.production.connectionString  = opt.database.connectionString;
   connOpt.production.password          = opt.database.password;
   connOpt.development                  = connOpt.production;

   return connOpt;
}

} // namespace lis
} // namespace tbs