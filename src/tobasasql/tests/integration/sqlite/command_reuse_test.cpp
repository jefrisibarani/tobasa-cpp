#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "tobasa_test_support/environment.h"
#include "tobasasql/sqlite_result.h"
#include "tobasasql/sqlite_command.h"
#include "tobasasql/sqlite_connection.h"
#include "tobasasql/sql_dataset.h"

using tbs::sql::DataType;
using tbs::sql::SqliteResult;
using tbs::sql::SqliteCommand;
using tbs::sql::SqliteConnection;
using tbs::sql::SqlParameter;
using tbs::sql::SqlParameterCollection;

TEST(SqliteCommandIntegrationTest, ReusesPreparedInsertForMultipleParameterSets)
{
   const std::string connectionString = tbs::test_support::loadEnvironmentValue(
      "TOBASA_SQLITE_TEST_CONNECTION", std::string(TOBASA_PROJECT_SOURCE_DIR) + "/.env");

   if (connectionString.empty())
      GTEST_SKIP() << "Set TOBASA_SQLITE_TEST_CONNECTION or add it to .env";

   SqliteConnection conn;
   ASSERT_TRUE(conn.connect(connectionString));

   ASSERT_EQ(conn.execute(
      "CREATE TEMP TABLE tbs_prepared_cmd_test ("
      "id INT NOT NULL, name VARCHAR(64) NOT NULL)"), 0);

   const std::vector<std::pair<int32_t, std::string>> rows = {
      {1, "Budi"}, {2, "Wati"}, {3, "Yanto"}
   };

   SqliteCommand cmd(&conn);

   ASSERT_TRUE(cmd.prepare("INSERT INTO tbs_prepared_cmd_test (id, name) VALUES ($1, $2)"));

   for (const auto& row : rows)
   {
      SqlParameterCollection parameters;
      parameters.emplace_back(std::make_shared<SqlParameter>("id",   DataType::integer, row.first));
      parameters.emplace_back(std::make_shared<SqlParameter>("name", DataType::varchar, row.second));

      ASSERT_TRUE(cmd.bind(parameters));
      EXPECT_EQ(cmd.execute(), 1);
      cmd.reset();
   }

   EXPECT_EQ(conn.executeScalar("SELECT COUNT(*) FROM tbs_prepared_cmd_test"), "3");

   // ---------------------------------------------------------------------------------

   ASSERT_TRUE(cmd.prepare("SELECT * FROM  tbs_prepared_cmd_test WHERE name = ?"));

   for (const std::string& value : { std::string("Budi"),std::string("Wati"),std::string("Yanto") } )
   {
      SqlParameterCollection parameters;
      parameters.emplace_back(std::make_shared<SqlParameter>("name", DataType::varchar, value));

      ASSERT_TRUE(cmd.bind(parameters));
      auto result = cmd.executeResult();
      ASSERT_TRUE(result != nullptr);
      ASSERT_EQ(result->totalRows(), 1);
      ASSERT_EQ(result->totalColumns(), 2);
      EXPECT_EQ(tbs::VariantHelper<>::toString(result->data().at(0).at(1)), value);

      cmd.reset();
   }

   // SQLite does not allow calling repeated execute*() MUST re-bind() and reset()
   // ---------------------------------------------------------------------------------
   cmd.reset();

   SqlParameterCollection parameters;
   parameters.emplace_back(std::make_shared<SqlParameter>("name", DataType::varchar, "Wati"));
   ASSERT_TRUE(cmd.bind(parameters));

   SqliteResult sqlres = cmd.executeSqlResult();
   sqlres.navigator().moveFirst();
   ASSERT_EQ(sqlres.columnNames().at(0), "id");
   ASSERT_EQ(sqlres.totalRows(), 1);
   ASSERT_EQ(sqlres.totalColumns(), 2);
   EXPECT_EQ(sqlres.getStringValue("name"), "Wati");
   EXPECT_EQ(sqlres.getStringValue("id"), "2");
   EXPECT_EQ(tbs::VariantHelper<>::toString(sqlres.getVariantValue("name")), "Wati");

   cmd.reset();

   ASSERT_TRUE(cmd.bind(parameters));
   auto res1 = cmd.execute();
   ASSERT_EQ(res1, 0);
   cmd.reset();

   ASSERT_TRUE(cmd.bind(parameters));
   auto res2 = cmd.executeScalar();
   ASSERT_EQ(res2, std::string("2"));
   cmd.reset();

   ASSERT_TRUE(cmd.bind(parameters));
   auto res3 = cmd.executeResult();
   EXPECT_EQ( tbs::VariantHelper<>::toString( res3->data().at(0).at(1)), "Wati");
   cmd.reset();

   ASSERT_TRUE(cmd.bind(parameters));
   auto res4 = cmd.execute();
   ASSERT_EQ(res4, 0);
   cmd.reset();

   ASSERT_TRUE(cmd.bind(parameters));
   auto res5 = cmd.executeScalar();
   ASSERT_EQ(res5, std::string("2"));
   cmd.reset();
   // ---------------------------------------------------------------------------------
}