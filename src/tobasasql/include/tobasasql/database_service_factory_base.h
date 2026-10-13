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
 * Base interface for creating and managing database services.
 *
 * This factory stores database connector settings, manages the default
 * connection pool size, and creates database services for configured
 * connections.
 */
class DbServiceFactoryBase : private NonCopyable
{
public:
   DbServiceFactoryBase() = default;
   virtual ~DbServiceFactoryBase() = default;

   /// Connect to the main application database using the configured settings.
   virtual bool connect()    = 0;

   /// Disconnect all active database connections and clear pooled resources.
   virtual bool disconnect() = 0;

   /// Return true if the main application database connection is currently active.
   virtual bool connected()  = 0;

   /// Register a database connector option.
   /// If poolSize is 0, this connector uses the global pool size.
   virtual void addConnectorOption(const std::string& name, const conf::ConnectorOption& option, int poolSize=0) = 0;
   
   /// Get the stored connector settings for the given name.
   /// The isValid flag is set to true if the option exists.
   virtual conf::ConnectorOption getConnectorOption(const std::string& name, bool& isValid) = 0;
   
   /// Create a database service for the given connector name.
   /// If pooled is true, the service uses a pooled connection; otherwise it gets a direct connection.
   virtual SqlServicePtr getDbService(const std::string& name, bool pooled = true) = 0;
   
   /// Set the default connection pool size for all connectors.
   virtual void setConnectionPoolSize(int size) = 0;

   /// Get the connection pointer variant used by the main database connection.
   virtual SqlConnectionPtrVariant&   sqlConnPtrVariant()   = 0;

   /// Get the SQL driver variant used by the main database connection.
   virtual SqlDriverVariant&          getSqlDriverVariant() = 0;
};

using DbServiceFactoryPtr = std::shared_ptr<DbServiceFactoryBase>;

} // namespace sql
} // namespace tbs
