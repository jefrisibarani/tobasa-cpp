#pragma once

#include <tobasa/exception.h>
#include <tobasasql/database_service_factory_base.h>
#include <tobasasql/sql_connection_pool.h>

namespace tbs {
namespace sql { 

/** 
 * \ingroup SQL
 * Database service
 */
class DbServiceFactory
   : public DbServiceFactoryBase
{
protected:
   int _connPoolSize;
   std::map<std::string, conf::ConnectorOption> _connectorOptions;
   ConnectionPoolManager _connPoolManager;
   // List of non-pooled connectors created
   std::vector<std::shared_ptr<DatabaseConnector>> _connectorList;

public:
   DbServiceFactory();
   virtual ~DbServiceFactory();

   /// Connect to database using settings provided in file appsettings.conf
   virtual bool connect();
   virtual bool disconnect();
   virtual bool connected();

   virtual void addConnectorOption(const std::string& name, const conf::ConnectorOption& option);
   virtual SqlServicePtr getDbService(const std::string& serviceName, bool pooled = true);

   virtual SqlConnectionPtrVariant& sqlConnPtrVariant();
   virtual SqlDriverVariant& getSqlDriverVariant();

   virtual bool testConnection();

   conf::ConnectorOption getConnectorOption(const std::string& name, bool& isValid);
   
   std::shared_ptr<DatabaseConnector> getConnector(const std::string& optionName, bool pooled=true);

   void setConnectionPoolSize(int size);

   template <template <class> class RepoType>
   SqlServicePtr createService(const std::string& optionName, bool pooled=true)
   {
      if (pooled)
      {
         bool valid = false;
         auto option = getConnectorOption(optionName, valid);
         if (!valid)
            throw AppException("Invalid connector option: " + optionName);

         const size_t poolSize = _connPoolSize;
         auto pool = _connPoolManager.getOrCreatePool(optionName, option, poolSize);
         auto pooledConn = pool->acquire();
         auto connector = pooledConn.conn();

         if (!connector || !connector->connected())
            throw AppException("Database connection unavailable for " + optionName);

         auto service = connector->createService<RepoType>();

         // -------------------------------
         // shared_ptr aliasing constructor
         // -------------------------------

         struct ServiceWithPool {
            SqlServicePtr repo;
            PooledConnection pooled;
         };

         auto container = std::make_shared<ServiceWithPool>();
         container->repo = service;
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

   virtual SqlServicePtr doGetDbService(const std::string& serviceName, bool pooled = true)
   {
      return nullptr;
   }   

};

} // namespace sql
} // namespace tbs