// Demonstrates the TobasaSQL connection pool behavior.
// A pooled connector is acquired from DbServiceFactory, used briefly, and then
// automatically returned to the pool when the shared_ptr is released.

#include <iostream>

#include <tobasa/datetime.h>
#include <tobasa/logger.h>
#include <tobasasql/database_service_factory.h>

int main()
{
   using namespace tbs;
   using namespace tbs::sql;

   Logger::setTarget(new log::CoutLogSink());

   conf::ConnectorOption connectorOption;
   connectorOption.environment = "development";
   connectorOption.logSqlQuery = false;
   connectorOption.logInternalSqlQuery = false;

   connectorOption.production.dbDriver = BackendType::sqlite;
   connectorOption.production.connectionString = "Database=./tbs_pool_demo.db3;OpenCreate=True;OpenMemory=False;";
   connectorOption.production.password = "";
   connectorOption.development = connectorOption.production;
   connectorOption.securitySalt = "";

   try
   {
      DbServiceFactory factory;
      factory.setConnectionPoolSize(2);
      factory.addConnectorOption("PoolDb", connectorOption);

      std::cout << "Connection pool configured with max size 2\n";

      {
         auto connA = factory.getConnector("PoolDb", true);
         auto connB = factory.getConnector("PoolDb", true);

         if (!connA || !connB)
         {
            std::cerr << "Could not acquire pooled connectors\n";
            return 1;
         }

         std::visit(
            [](const auto& connection)
            {
               connection->executeVoid("CREATE TABLE IF NOT EXISTS pool_demo (id INTEGER PRIMARY KEY, label TEXT)");
            },
            connA->sqlConnPtrVariant());

         std::visit(
            [](const auto& connection)
            {
               connection->executeVoid("DELETE FROM pool_demo");
               connection->executeVoid("INSERT INTO pool_demo(label) VALUES ('alpha')");
            },
            connA->sqlConnPtrVariant());

         std::visit(
            [](const auto& connection)
            {
               std::cout << "Connection A value: "
                         << connection->executeScalar("SELECT COUNT(*) FROM pool_demo")
                         << "\n";
            },
            connA->sqlConnPtrVariant());

         std::visit(
            [](const auto& connection)
            {
               std::cout << "Connection B value: "
                         << connection->executeScalar("SELECT 1")
                         << "\n";
            },
            connB->sqlConnPtrVariant());

         std::cout << "Both pooled connectors are active in this scope.\n";
      }

      std::cout << "When the scoped shared_ptrs are released, the connectors are returned to the pool.\n";

      {
         auto reused = factory.getConnector("PoolDb", true);
         std::visit(
            [](const auto& connection)
            {
               std::cout << "Reused pooled connection: "
                         << connection->executeScalar("SELECT COUNT(*) FROM pool_demo")
                         << "\n";
            },
            reused->sqlConnPtrVariant());
      }

      std::cout << "Pool demonstration completed successfully\n";
      factory.disconnect();
   }
   catch (const std::exception& ex)
   {
      std::cerr << "Pool sample error: " << ex.what() << "\n";
      return 1;
   }

   return 0;
}
