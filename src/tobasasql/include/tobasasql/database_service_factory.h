#pragma once

#include "tobasasql/exp_imp.h"

#include <tobasa/exception.h>
#include <tobasasql/database_service_factory_base.h>
#include <tobasasql/sql_connection_pool.h>

namespace tbs {
namespace sql { 

/**
 * \ingroup SQL
 * Concrete database service factory.
 *
 * This class keeps the configured connector options, manages the shared
 * connection pool, and creates database services for the configured
 * database connections.
 */
class TBS_SQL_API DbServiceFactory
   : public DbServiceFactoryBase
{
protected:
   int _connPoolSize;

   struct ConnectorData
   {
      int poolSize;
      conf::ConnectorOption option;
   };

   std::map<std::string, ConnectorData > _connectorOptions;
   ConnectionPoolManager _connPoolManager;

   // List of non-pooled connectors created
   std::vector<std::shared_ptr<DatabaseConnector>> _connectorList;

public:
   DbServiceFactory();
   virtual ~DbServiceFactory();

   /// Connect to the main configured database.
   virtual bool connect();

   /// Close all active connections and clear the pool state.
   virtual bool disconnect();

   /// Return true if the main configured database is connected.
   virtual bool connected();

   /// Register a named connector configuration.
   /// If poolSize is 0, the connector uses the global pool size.
   virtual void addConnectorOption(const std::string& name, const conf::ConnectorOption& option, int poolSize=0);

   /// Create a database service for the named connector.
   /// If pooled is true, the service uses a pooled connection.
   virtual SqlServicePtr getDbService(const std::string& serviceName, bool pooled = true);

   /// Get the connection pointer variant for the main database connection.
   virtual SqlConnectionPtrVariant& sqlConnPtrVariant();

   /// Get the SQL driver variant for the main database connection.
   virtual SqlDriverVariant& getSqlDriverVariant();

   /// Test whether the main database connection is healthy.
   virtual bool testConnection();

   /// Return the stored connector settings for the given name.
   /// The isValid flag tells whether the option exists.
   conf::ConnectorOption getConnectorOption(const std::string& name, bool& isValid);
   
   /// Get a connector for the named option.
   std::shared_ptr<DatabaseConnector> getConnector(const std::string& optionName, bool pooled=true);

   /// Get a connector and wait for an available pooled connection using the given options.
   std::shared_ptr<DatabaseConnector> getConnector(const std::string& optionName, bool pooled, const PoolAcquireOptions& acquireOptions);

   /// Set the default pool size used by connectors that do not define their own size.
   void setConnectionPoolSize(int size);

   /// Create a repository service for the named database option.
   template <template <class> class RepoType>
   SqlServicePtr createService(const std::string& optionName, bool pooled=true)
   {
      return createService<RepoType>(optionName, pooled, PoolAcquireOptions{});
   }

   /// Create a repository service using the given connection-acquire options.
   template <template <class> class RepoType>
   SqlServicePtr createService(const std::string& optionName, bool pooled, const PoolAcquireOptions& acquireOptions)
   {
      if (pooled)
      {
         bool valid = false;
         auto data  = getConnectorData(optionName, valid);
         if (!valid)
            throw AppException("Invalid connector option: " + optionName);

         const size_t poolSize = data.poolSize > 0 ? data.poolSize : _connPoolSize;
         auto pool       = _connPoolManager.getOrCreatePool(optionName, data.option, poolSize);
         auto pooledConn = pool->acquire(acquireOptions);
         auto connector  = pooledConn.connector();

         if (!connector || !connector->connected())
            throw AppException("Database connection unavailable for " + optionName);

         auto service = connector->createService<RepoType>();

         // -------------------------------
         // shared_ptr aliasing constructor
         // -------------------------------

         struct ServiceWithPool 
         {
            SqlServicePtr repo;
            PooledConnection pooled;
         };

         auto container    = std::make_shared<ServiceWithPool>();
         container->repo   = service;
         container->pooled = std::move(pooledConn);

         return SqlServicePtr(container, container->repo.get());
      }
      else
      {
         // Non-pooled connection
         auto connector = getConnector(optionName,false);
         auto service = connector->createService<RepoType>();
         return service;
      }
   }

protected:

   ConnectorData getConnectorData(const std::string& name, bool& isValid);

   virtual SqlServicePtr doGetDbService(const std::string& serviceName, bool pooled = true)
   {
      return nullptr;
   }   

};

} // namespace sql
} // namespace tbs