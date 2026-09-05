#include <iostream>
#include <string>
#include <vector>
#include <tobasa/datetime.h>
#include <tobasa/logger.h>
#include <tobasasql/database_service_factory.h>
#include <tobasasql/sql_connection.h>
#include <tobasasql/sql_driver.h>
#include <tobasasql/sql_query.h>
#include <tobasasql/sql_service_base.h>

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
#include <objbase.h>
#endif

namespace sample {

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
class ComInitialization
{
private:
   bool _initialized = false;

public:
   explicit ComInitialization(bool required)
   {
      if (required)
      {
         HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
         if (FAILED(result))
            throw std::runtime_error("Could not initialize COM for ADO");

         _initialized = true;
      }
   }

   ~ComInitialization()
   {
      if (_initialized)
         CoUninitialize();
   }
};
#endif



class UserServiceBase : public tbs::sql::SqlServiceBase
{
public:
   UserServiceBase(const UserServiceBase &) = delete;
   UserServiceBase(UserServiceBase &&) = delete;

   UserServiceBase() {}
   virtual ~UserServiceBase() {}

   virtual std::vector<std::string> getUserNames() = 0;
   virtual bool deleteUser(const std::string& userName) = 0;
   virtual bool addUser(const std::string& userName, int userLevel) = 0;
};
using UserServicePtr = std::shared_ptr<UserServiceBase>;


template <typename SqlDriverType>
class UserService : public UserServiceBase
{
private:
   using SqlConnection = tbs::sql::SqlConnection<SqlDriverType>;
   using SqlQuery = tbs::sql::SqlQuery<SqlDriverType>;

   SqlConnection& _connection;

public:
   explicit UserService(SqlConnection& connection)
      : _connection(connection)
   {
      if (!_connection.executeVoid(
            "CREATE TABLE IF NOT EXISTS users ("
            "user_name TEXT PRIMARY KEY, user_level INTEGER NOT NULL)"))
      {
         throw std::runtime_error("Could not create users table");
      }
   }

   bool databaseConnected() override
   {
      return _connection.status() == tbs::sql::ConnectionStatus::ok
         && _connection.executeScalar("SELECT 1") == "1";
   }

   std::vector<std::string> getUserNames()
   {
      SqlQuery query(_connection, "SELECT user_name FROM users ORDER BY user_name");
      auto result = query.executeResult();
      std::vector<std::string> userNames;

      if (result && result->isValid())
      {
         for (long row = 0; row < result->totalRows(); ++row)
         {
            result->locate(static_cast<int>(row));
            userNames.push_back(result->getStringValue("user_name"));
         }
      }

      return userNames;
   }

   bool deleteUser(const std::string& userName)
   {
      SqlQuery query(_connection, "DELETE FROM users WHERE user_name = :user_name");
      query.addParam("user_name", tbs::sql::DataType::varchar, userName);
      return query.executeVoid();
   }

   bool addUser(const std::string& userName, int userLevel)
   {
      SqlQuery query(_connection,
         "INSERT INTO users (user_name, user_level) "
         "VALUES (:user_name, :user_level)");

      query.addParam("user_name",  tbs::sql::DataType::varchar, userName);
      query.addParam("user_level", tbs::sql::DataType::integer, userLevel);

      return query.executeVoid();
   }
};

} // namespace sample

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

   connectorOption.production.dbDriver = BackendType::sqlite;
   connectorOption.production.connectionString = "Database=./tbs_UserService.db3;OpenCreate=True;OpenMemory=False;";
   connectorOption.production.password = "";
   connectorOption.development = connectorOption.production;
   // tell DatabaseConnector that we are using clear password
   connectorOption.securitySalt = "";

   try
   {
#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
      sample::ComInitialization comInitialization(
         connectorOption.production.dbDriver == BackendType::adodb);
#endif

      DbServiceFactory factory;
      factory.setConnectionPoolSize(1);
      factory.addConnectorOption("MainDb", connectorOption);

      // createService initializes the configured connector and supplies its
      // typed connection to the service implementation.
      SqlServicePtr svc = factory.createService<sample::UserService>("MainDb", false);
      sample::UserServicePtr userService = std::static_pointer_cast<sample::UserServiceBase>(svc);

      std::cout << "Service id: " << userService->id() << "\n";
      std::cout << "Service database connected: "
                << std::boolalpha << userService->databaseConnected() << "\n";

      if (!userService->databaseConnected())
      {
         std::cerr << "Database service connection test failed\n";
         return 1;
      }

      // Remove the sample rows first so the program can be run repeatedly.
      userService->deleteUser("Ada");
      userService->deleteUser("Grace");
      
      if ( !userService->addUser("Ada", 1) || !userService->addUser("Grace", 2) )
      {
         std::cerr << "Could not add sample users\n";
         return 1;
      }

      std::cout << "Users:\n";
      for (const auto& userName : userService->getUserNames())
         std::cout << "  " << userName << "\n";

      if (!userService->deleteUser("Grace"))
      {
         std::cerr << "Could not delete sample user\n";
         return 1;
      }

      if (!userService->beginTransaction())
      {
         std::cerr << "Could not begin transaction\n";
         return 1;
      }

      if (!userService->rollbackTransaction())
      {
         std::cerr << "Could not roll back transaction\n";
         return 1;
      }

      std::cout << "Database service is ready\n";
      factory.disconnect();
   }
   catch (const std::exception& ex)
   {
      std::cerr << "Database service error: " << ex.what() << "\n";
      return 1;
   }

   return 0;
}
