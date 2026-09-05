#include <iostream>
#include <tobasa/datetime.h>
#include <tobasa/logger.h>
#include <tobasasql/database_connector.h>


int main()
{
   using namespace tbs;
   using namespace tbs::sql;

   // We need this for Tobasa DateTime objects or SQL date/time conversion:
   if (! tbs::DateTime::initTimezoneData())
      return 1;

   Logger::setTarget(new log::CoutLogSink());

   conf::ConnectorOption connectorOption;
   connectorOption.environment = "development";
   connectorOption.logSqlQuery = true;
   connectorOption.logInternalSqlQuery = false;
   // The selected backend is configuration data, not a connection template.
   connectorOption.production.dbDriver = BackendType::sqlite;
   connectorOption.production.connectionString = "Database=./tbs_connector.db3;OpenCreate=True;OpenMemory=False;";
   connectorOption.production.password = "";
   connectorOption.development = connectorOption.production;
   // tell DatabaseConnector that we are using clear password
   connectorOption.securitySalt = "";

   try
   {
      DatabaseConnector connector(connectorOption, "ConnectorSample");
      connector.initSqlDriver();

      if (!connector.connect())
      {
         std::cerr << "Could not connect to the configured database\n";
         return 1;
      }

      if (!connector.testConnection())
      {
         std::cerr << "Database connection test failed\n";
         return 1;
      }

      std::visit(
         [](const auto& connection)
         {
            std::cout << "Connected to "
                      << backendTypeToString(connection->backendType())
                      << "\n";
            std::cout << "Backend version: "
                      << connection->versionString()
                      << "\n";
            std::cout << "SELECT 1: "
                      << connection->executeScalar("SELECT 1")
                      << "\n";
         },
         connector.sqlConnPtrVariant());

      connector.disconnect();
   }
   catch (const std::exception& ex)
   {
      std::cerr << "Connector error: " << ex.what() << "\n";
      return 1;
   }

   return 0;
}
