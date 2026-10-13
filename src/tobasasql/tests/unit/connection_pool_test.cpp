#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <future>
#include <memory>
#include <string>
#include <thread>

#include <tobasasql/db_api.h>
#include <tobasasql\database_service_factory.h>
#include <tobasasql\sql_connection.h>
#include <tobasasql\sql_connection_pool.h>

namespace tbs::sql::detail {

class ConnectionPoolTestAccess
{
public:
   static bool waitForWaiterCount(ConnectionPool& pool, size_t count,
      std::chrono::milliseconds timeout = std::chrono::seconds(2))
   {
      const auto deadline = std::chrono::steady_clock::now() + timeout;
      do
      {
         {
            std::lock_guard lock(pool._mutex);
            if (pool.waitState()->waiters.size() >= count)
               return true;
         }
         std::this_thread::yield();
      }
      while (std::chrono::steady_clock::now() < deadline);

      return false;
   }
};

} // namespace tbs::sql::detail

namespace {

using namespace tbs;
using namespace tbs::sql;

DbConnectionOptions makeSqliteOptions()
{
   DbConnectionOptions options;
   options.backend = BackendType::sqlite;
   options.connectionString = "OpenMemory=True;";
   return options;
}

std::filesystem::path makePoolTestDirectory()
{
   const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
   return std::filesystem::temp_directory_path()
      / ("tobasa_sql_pool_test_" + std::to_string(suffix));
}

conf::ConnectorOption makeSqlitePoolOptions(const std::string& connectionString)
{
   conf::ConnectorOption options{};
   options.environment = "production";
   options.production.dbDriver = BackendType::sqlite;
   options.production.connectionString = connectionString;
   options.development = options.production;
   options.connectionStringIsComplete = true;
   return options;
}

template <typename Driver>
class PoolProbeService final : public SqlServiceBase
{
public:
   explicit PoolProbeService(SqlConnection<Driver>&) {}
};

TEST(ConnectionPoolTest, PooledConnectionSupportsCommandsAndResultLifetime)
{
   DbConnectionPool pool("db_api_test", makeSqliteOptions(), 1);
   DbResult result;

   {
      auto connection = pool.acquire();
      ASSERT_TRUE(connection);
      EXPECT_EQ(connection.createCommand("CREATE TABLE people (id INTEGER PRIMARY KEY, name TEXT NOT NULL)").execute(), 0);

      auto insert = connection.createCommand("INSERT INTO people (name) VALUES (:name)");
      insert.addParam("name", DataType::varchar, std::string("Ada"), 32);
      EXPECT_EQ(insert.execute(), 1);

      result = connection.createCommand("SELECT id, name FROM people ORDER BY id").executeQuery();
   }

   ASSERT_TRUE(result.isValid());
   EXPECT_EQ(result.totalRows(), 1);
   result.moveFirst();
   EXPECT_FALSE(result.isEof());
   EXPECT_EQ(result.getInt64("id"), 1);
   EXPECT_EQ(result.getString("name"), "Ada");

   result = DbResult();
   auto reusedConnection = pool.acquire();
   EXPECT_EQ(reusedConnection.createCommand("SELECT COUNT(*) FROM people").executeScalar(), "1");
}

TEST(ConnectionPoolTest, InitialConnectionFailureDoesNotConsumePoolCapacity)
{
   const auto testDirectory = makePoolTestDirectory();
   const auto databasePath = testDirectory / "database" / "pool.sqlite";
   auto pool = std::make_shared<ConnectionPool>(
      "initial_failure_test",
      makeSqlitePoolOptions("Database=" + databasePath.string() + ";OpenCreate=True;"), 2);

   EXPECT_THROW((void)pool->acquire(), AppException);
   std::filesystem::create_directories(databasePath.parent_path());

   auto first = pool->acquire();
   ASSERT_TRUE(first.connector()->connected());

   auto secondAcquire = std::async(std::launch::async, [pool]
   {
      return pool->acquire();
   });
   const bool acquiredBeforeReleasingFirst
      = secondAcquire.wait_for(std::chrono::seconds(5)) == std::future_status::ready;
   if (!acquiredBeforeReleasingFirst)
      first = PooledConnection{};

   EXPECT_TRUE(acquiredBeforeReleasingFirst);
   auto second = secondAcquire.get();
   EXPECT_TRUE(second.connector()->connected());

   second = PooledConnection{};
   first = PooledConnection{};
   pool.reset();
   std::error_code ignored;
   std::filesystem::remove_all(testDirectory, ignored);
}

TEST(ConnectionPoolTest, FailedIdleReconnectDiscardsConnectorAndFreesCapacity)
{
   const auto testDirectory = makePoolTestDirectory();
   const auto databasePath = testDirectory / "database" / "pool.sqlite";
   std::filesystem::create_directories(databasePath.parent_path());
   auto pool = std::make_shared<ConnectionPool>(
      "reconnect_failure_test",
      makeSqlitePoolOptions("Database=" + databasePath.string() + ";OpenCreate=True;"), 1);

   {
      auto connection = pool->acquire();
      ASSERT_TRUE(connection.connector()->connected());
      ASSERT_TRUE(connection.connector()->disconnect());
   }

   std::filesystem::remove_all(databasePath.parent_path());
   EXPECT_THROW((void)pool->acquire(), AppException);

   std::filesystem::create_directories(databasePath.parent_path());
   auto recovered = pool->acquire();
   EXPECT_TRUE(recovered.connector()->connected());

   recovered = PooledConnection{};
   pool.reset();
   std::error_code ignored;
   std::filesystem::remove_all(testDirectory, ignored);
}

TEST(ConnectionPoolTest, ReusingPoolNameWithDifferentSettingsThrows)
{
   ConnectionPoolManager manager;
   const auto original = makeSqlitePoolOptions("OpenMemory=True;");
   const auto different = makeSqlitePoolOptions("OpenMemory=True;Mode=Memory;Cache=Shared");

   auto firstPool = manager.getOrCreatePool("name_reuse_test", original, 1);
   ASSERT_TRUE(firstPool);

   EXPECT_THROW((void)manager.getOrCreatePool("name_reuse_test", different, 1), std::invalid_argument);
   EXPECT_THROW((void)manager.getOrCreatePool("name_reuse_test", original, 2), std::invalid_argument);
}

TEST(ConnectionPoolTest, PoolAcquireTimeoutDoesNotConsumeCapacity)
{
   auto pool = std::make_shared<ConnectionPool>(
      "timeout_test", makeSqlitePoolOptions("OpenMemory=True;"), 1);
   auto held = pool->acquire();

   PoolAcquireOptions options;
   options.timeout = std::chrono::milliseconds(25);
   EXPECT_THROW((void)pool->acquire(options), PoolAcquireTimeoutError);

   held = PooledConnection{};
   auto recovered = pool->acquire();
   EXPECT_TRUE(recovered.connector()->connected());
}

TEST(ConnectionPoolTest, CancellationOnlyStopsRequestsUsingItsToken)
{
   auto pool = std::make_shared<ConnectionPool>(
      "cancel_test", makeSqlitePoolOptions("OpenMemory=True;"), 1);
   auto held = pool->acquire();

   PoolAcquireCancellationSource cancelledSource;
   PoolAcquireCancellationSource activeSource;
   PoolAcquireOptions cancelledOptions;
   cancelledOptions.timeout = std::chrono::seconds(5);
   cancelledOptions.cancellationToken = cancelledSource.token();
   PoolAcquireOptions activeOptions;
   activeOptions.timeout = std::chrono::seconds(5);
   activeOptions.cancellationToken = activeSource.token();

   auto cancelledWaiter = std::async(std::launch::async, [&]
   {
      try
      {
         auto lease = pool->acquire(cancelledOptions);
         return false;
      }
      catch (const PoolAcquireCancelledError&)
      {
         return true;
      }
   });
   ASSERT_TRUE(detail::ConnectionPoolTestAccess::waitForWaiterCount(*pool, 1));

   auto activeWaiter = std::async(std::launch::async, [&]
   {
      try
      {
         auto lease = pool->acquire(activeOptions);
         return lease.connector() && lease.connector()->connected();
      }
      catch (...)
      {
         return false;
      }
   });
   ASSERT_TRUE(detail::ConnectionPoolTestAccess::waitForWaiterCount(*pool, 2));

   cancelledSource.cancel();
   ASSERT_EQ(cancelledWaiter.wait_for(std::chrono::seconds(2)), std::future_status::ready);
   EXPECT_TRUE(cancelledWaiter.get());

   held = PooledConnection{};
   ASSERT_EQ(activeWaiter.wait_for(std::chrono::seconds(2)), std::future_status::ready);
   EXPECT_TRUE(activeWaiter.get());
}

TEST(ConnectionPoolTest, PoolAdmissionIsFifo)
{
   auto pool = std::make_shared<ConnectionPool>("fifo_test", makeSqlitePoolOptions("OpenMemory=True;"), 1);
   auto held = pool->acquire();
   PoolAcquireOptions options;
   options.timeout = std::chrono::seconds(5);

   auto firstWaiter = std::async(std::launch::async, [&]
   {
      try
      {
         return pool->acquire(options);
      }
      catch (...)
      {
         return PooledConnection{};
      }
   });
   ASSERT_TRUE(detail::ConnectionPoolTestAccess::waitForWaiterCount(*pool, 1));

   auto secondWaiter = std::async(std::launch::async, [&]
   {
      try
      {
         return pool->acquire(options);
      }
      catch (...)
      {
         return PooledConnection{};
      }
   });
   ASSERT_TRUE(detail::ConnectionPoolTestAccess::waitForWaiterCount(*pool, 2));

   held = PooledConnection{};
   ASSERT_EQ(firstWaiter.wait_for(std::chrono::seconds(2)), std::future_status::ready);
   auto first = firstWaiter.get();
   ASSERT_TRUE(first.connector());
   EXPECT_EQ(secondWaiter.wait_for(std::chrono::milliseconds(0)), std::future_status::timeout);

   first = PooledConnection{};
   ASSERT_EQ(secondWaiter.wait_for(std::chrono::seconds(2)), std::future_status::ready);
   auto second = secondWaiter.get();
   EXPECT_TRUE(second.connector());
}

TEST(ConnectionPoolTest, DbConnectionPoolForwardsAcquireOptions)
{
   DbConnectionPool pool("db_api_timeout_test", makeSqliteOptions(), 1);
   auto held = pool.acquire();
   PoolAcquireOptions options;
   options.timeout = std::chrono::milliseconds(25);

   EXPECT_THROW((void)pool.acquire(options), PoolAcquireTimeoutError);
}

TEST(ConnectionPoolTest, DbServiceFactoryForwardsAcquireOptions)
{
   DbServiceFactory factory;
   factory.setConnectionPoolSize(1);
   factory.addConnectorOption("factory_acquire_test",
      makeSqlitePoolOptions("OpenMemory=True;"));

   PoolAcquireOptions options;
   options.timeout = std::chrono::milliseconds(25);

   auto connector = factory.getConnector("factory_acquire_test", true);
   EXPECT_THROW( (void)factory.getConnector("factory_acquire_test", true, options), PoolAcquireTimeoutError);
   connector.reset();

   auto service = factory.createService<PoolProbeService>("factory_acquire_test", true);
   ASSERT_TRUE(service);
   EXPECT_THROW( (void)factory.createService<PoolProbeService>("factory_acquire_test", true, options), PoolAcquireTimeoutError);
   service.reset();
   EXPECT_TRUE(factory.disconnect());
}

} // namespace
