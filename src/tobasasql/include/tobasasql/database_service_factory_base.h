#pragma once

#include <tobasa/non_copyable.h>
#include <tobasasql/sql_driver.h>
#include <tobasasql/sql_conn_variant.h>
#include <tobasasql/sql_service_base.h>
#include <tobasasql/settings.h>

namespace tbs {
namespace sql {


/** 
 * \ingroup SQL
 * DbServiceFactoryBase
 */
class DbServiceFactoryBase : private NonCopyable
{
public:
   DbServiceFactoryBase() = default;
   virtual ~DbServiceFactoryBase() = default;

   /// Connect to database using settings provided in file appsettings.conf
   virtual bool connect()       = 0;
   virtual bool disconnect()    = 0;
   virtual bool connected()     = 0;

   virtual void addConnectorOption(const std::string& name, const conf::ConnectorOption& option) = 0;
   virtual conf::ConnectorOption getConnectorOption(const std::string& name, bool& isValid) = 0;
   virtual SqlServicePtr getDbService(const std::string& name, bool pooled = true) = 0;
   virtual void setConnectionPoolSize(int size) = 0;

   virtual SqlConnectionPtrVariant&   sqlConnPtrVariant()   = 0;
   virtual SqlDriverVariant&          getSqlDriverVariant() = 0;
};

using DbServiceFactoryPtr = std::shared_ptr<DbServiceFactoryBase>;

} // namespace sql
} // namespace tbs
