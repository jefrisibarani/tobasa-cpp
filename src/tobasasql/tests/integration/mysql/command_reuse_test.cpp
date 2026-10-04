#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "tobasa_test_support/environment.h"
#include "tobasasql/mysql_result.h"
#include "tobasasql/mysql_command.h"
#include "tobasasql/mysql_connection.h"
#include "tobasasql/sql_dataset.h"

namespace {

using tbs::sql::DataType;
using tbs::sql::MysqlCommand;
using tbs::sql::MysqlResult;
using tbs::sql::MysqlConnection;
using tbs::sql::MysqlParameter;
using tbs::sql::MysqlParameterCollection;

TEST(MysqlCommandIntegrationTest, ReusesPreparedInsertForMultipleParameterSets)
{
   const std::string connectionString = tbs::test_support::loadEnvironmentValue(
         "TOBASA_MYSQL_TEST_CONNECTION", std::string(TOBASA_PROJECT_SOURCE_DIR) + "/.env" );

   if (connectionString.empty())
      GTEST_SKIP() << "Set TOBASA_MYSQL_TEST_CONNECTION or add it to .env";

   MysqlConnection conn;

   ASSERT_TRUE(conn.connect(connectionString));

   ASSERT_EQ(conn.execute(
      "CREATE TEMPORARY TABLE tbs_prepared_cmd_test ("
      "id INT NOT NULL, name VARCHAR(64) NOT NULL)"), 0);

   MysqlCommand cmd(&conn);

   ASSERT_TRUE(cmd.prepare(
      "INSERT INTO tbs_prepared_cmd_test (id, name) VALUES (?, ?)"));

   const std::vector<std::pair<int32_t, std::string>> rows = {
      {1, "Budi"}, {2, "Wati"}, {3, "Yanto"}
   };

   for (const auto& row : rows)
   {
      MysqlParameterCollection parameters;
      parameters.emplace_back(std::make_shared<MysqlParameter>("id",   DataType::integer, row.first));
      parameters.emplace_back(std::make_shared<MysqlParameter>("name", DataType::varchar, row.second));

      ASSERT_TRUE(cmd.bind(parameters));
      EXPECT_EQ(cmd.execute(), 1);
      cmd.reset();
   }

   EXPECT_EQ(conn.executeScalar(
      "SELECT COUNT(*) FROM tbs_prepared_cmd_test"), "3");

   // ---------------------------------------------------------------------------------

   ASSERT_TRUE(cmd.prepare("SELECT * FROM tbs_prepared_cmd_test WHERE name = ?"));

   for (const std::string& value : { std::string("Budi"),std::string("Wati"),std::string("Yanto") } )
   {
      MysqlParameterCollection parameters;
      parameters.emplace_back(std::make_shared<MysqlParameter>("name", DataType::varchar, value));

      ASSERT_TRUE(cmd.bind(parameters));
      auto result = cmd.executeResult();
      ASSERT_TRUE(result != nullptr);
      ASSERT_EQ(result->totalRows(), 1);
      ASSERT_EQ(result->totalColumns(), 2);
      EXPECT_EQ(tbs::MysqlVariantHelper::toString(result->data().at(0).at(1)), value);

      cmd.reset();
   }


   // MySQL allow Calling repeated execute*() without re-bind() and reset()
   // ---------------------------------------------------------------------------------
   cmd.reset();

   MysqlParameterCollection parameters;
   parameters.emplace_back(std::make_shared<MysqlParameter>("name", DataType::varchar, "Wati"));
   ASSERT_TRUE(cmd.bind(parameters));

   MysqlResult sqlres = cmd.executeSqlResult();
   sqlres.navigator().moveFirst();
   ASSERT_EQ(sqlres.columnNames().at(0), "id");
   ASSERT_EQ(sqlres.totalRows(), 1);
   ASSERT_EQ(sqlres.totalColumns(), 2);
   EXPECT_EQ(sqlres.getStringValue("name"), "Wati");
   EXPECT_EQ(sqlres.getStringValue("id"), "2");
   EXPECT_EQ(tbs::MysqlVariantHelper::toString(sqlres.getVariantValue("name")), "Wati");

   auto res1 = cmd.execute();
   ASSERT_EQ(res1, 0);

   auto res2 = cmd.executeScalar();
   ASSERT_EQ(res2, std::string("2"));

   auto res3 = cmd.executeResult();
   EXPECT_EQ( tbs::MysqlVariantHelper::toString( res3->data().at(0).at(1)), "Wati");

   auto res4 = cmd.execute();
   ASSERT_EQ(res4, 0);

   auto res5 = cmd.executeScalar();
   ASSERT_EQ(res5, std::string("2"));
   // ---------------------------------------------------------------------------------

}

} // namespace