#include <tobasa/config.h>
#include <tobasa/crypt.h>
#include <tobasasql/database_connector.h>
#include <tobasasql/database_service_factory.h>

namespace tbs {
namespace sql {

DbServiceFactory::DbServiceFactory() 
   : _connPoolSize(4)
{
}

DbServiceFactory::~DbServiceFactory()
{
   disconnect();
}

//! Connect to database using settings provided in file appsettings.conf
bool DbServiceFactory::connect()
{
   return getConnector("MainAppDbOption", false)->connected();
}

bool DbServiceFactory::disconnect()
{
   // Disconnect and clear non-pooled connectors
   {
      try
      {
         for (auto connector: _connectorList)
         {
            if (connector)
            {
               connector->disconnect();
               //delete connector;
               connector = nullptr;
            }
         }
      }
      catch (const std::exception& ex)
      {
         Logger::logE("{}", ex.what());
         return false;
      }
      _connectorList.clear();
   }

   // Clear connection pools
   {
      try {
         _connPoolManager.clear();
      }
      catch (const std::exception& ex) 
      {
         Logger::logE("DbServiceFactory::disconnect: {}", ex.what());
         return false;
      }
   }

   return true;
}

bool DbServiceFactory::connected()
{
   try
   {
      return getConnector("MainAppDbOption", false)->connected();
   }
   catch(const std::exception& ex)
   {
      Logger::logE("{}", ex.what());
   }
   return false;
}

void DbServiceFactory::addConnectorOption(const std::string& name, const conf::ConnectorOption& option)
{
   _connectorOptions[name] = option;
}

conf::ConnectorOption DbServiceFactory::getConnectorOption(const std::string& name, bool &isValid)
{
   auto it = _connectorOptions.find(name);
   if ( it == _connectorOptions.end() ) 
   {
      isValid = false;
      return conf::ConnectorOption();
   }
   else 
   {
      isValid = true;
      return it->second;
   }
}

void DbServiceFactory::setConnectionPoolSize(int size)
{
   _connPoolSize = size;
}

SqlServicePtr DbServiceFactory::getDbService(const std::string& serviceName, bool pooled)
{
   return doGetDbService(serviceName, pooled);  
}

SqlConnectionPtrVariant& DbServiceFactory::sqlConnPtrVariant()
{
   return getConnector("MainAppDbOption", false)->sqlConnPtrVariant();
}

SqlDriverVariant& DbServiceFactory::getSqlDriverVariant()
{
   return getConnector("MainAppDbOption", false)->getSqlDriverVariant();
}

bool DbServiceFactory::testConnection()
{
   auto conn = getConnector("MainAppDbOption", false);
   if (conn) {
      return conn->testConnection();
   }
   return false;
}

std::shared_ptr<DatabaseConnector> DbServiceFactory::getConnector(const std::string& optionName, bool pooled)
{
   if (pooled)
   {
      bool valid = false;
      auto option = getConnectorOption(optionName, valid);
         if (!valid)
            throw std::runtime_error("Invalid connector option: " + optionName);

      const size_t poolSize = _connPoolSize;
      auto pool = _connPoolManager.getOrCreatePool(optionName, option, poolSize);
      auto pooledConn = pool->acquire();
      auto connector = pooledConn.conn();
      if (!connector || !connector->connected())
         throw std::runtime_error("Database connection unavailable for " + optionName);

      DatabaseConnector* raw = connector.get();

      // Create a shared_ptr that points to the raw connector but keeps the pooledConn handle
      // alive inside the deleter. When the shared_ptr is destroyed, the deleter's captured
      // 'pooledConn' will be destroyed and the connection will be returned to the pool.
      auto deleter = [pooledConn = std::move(pooledConn)](DatabaseConnector* /*ptr*/) mutable {
         // no-op: pooledConn destructor will release the connection back to the pool
         // Do not delete ptr here.
      };

      return std::shared_ptr<DatabaseConnector>(raw, std::move(deleter));
   }
   else
   {
      // Non-pooled connection
      std::shared_ptr<DatabaseConnector> connector = nullptr;
      for (auto conn : _connectorList)
      {
         if (conn && conn->name() == optionName)
         {
            connector = conn;
            break;
         }
      }

      if (!connector)
      {
         bool valid = false;
         auto option = getConnectorOption(optionName, valid);
         if (!valid)
            throw AppException("Connector option " + optionName + " not found");

         // Create new non-pooled connector
         connector = std::make_shared<DatabaseConnector>(option, optionName);
         connector->initSqlDriver();
         if (!connector->connect())
            throw AppException("Failed to connect to database with option " + optionName);

         _connectorList.push_back(connector);
      }
      else
      {
         if (!connector->connected())
         {
            if (!connector->connect())
               throw AppException("Failed to connect database for " + optionName);
         }
      }

      return connector;
   }
}


} // namespace sql
} // namespace tbs