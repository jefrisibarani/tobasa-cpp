#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "tobasa_test_support/environment.h"
#include "tobasasql/adodb_result.h"
#include "tobasasql/adodb_command.h"
#include "tobasasql/adodb_connection.h"
#include "tobasasql/com_variant_helper.h"
#include "tobasasql/com_initializer.h"
#include "tobasasql/sql_dataset.h"

using tbs::sql::DataType;
using tbs::sql::AdodbResult;
using tbs::sql::AdodbCommand;
using tbs::sql::AdodbConnection;
using tbs::sql::AdoParameter;
using tbs::sql::AdoParameterCollection;

TEST(AdodbCommandIntegrationTest, ReusesPreparedInsertForMultipleParameterSets)
{
   const std::string connectionString = tbs::test_support::loadEnvironmentValue(
      "TOBASA_ADODB_TEST_CONNECTION_SQLNCLI", std::string(TOBASA_PROJECT_SOURCE_DIR) + "/.env");

   if (connectionString.empty())
      GTEST_SKIP() << "Set TOBASA_ADODB_TEST_CONNECTION_SQLNCLI or add it to .env";

   tbs::ComInitializer comInitializer(true);

   AdodbConnection conn;

   ASSERT_TRUE(conn.connect(connectionString));

   ASSERT_EQ(conn.execute(
      "CREATE TABLE ##tbs_prepared_cmd_test ("
      "id INT NOT NULL, name VARCHAR(64) NOT NULL)"), 0);

   AdodbCommand cmd(&conn);

   ASSERT_TRUE(cmd.prepare("INSERT INTO ##tbs_prepared_cmd_test (id, name) VALUES (?, ?)"));

   const std::vector<std::pair<int32_t, std::string>> rows = {
      {1, "Budi"}, {2, "Wati"}, {3, "Yanto"}
   };

   for (const auto& row : rows)
   {
      AdoParameterCollection parameters;
      parameters.emplace_back(std::make_shared<AdoParameter>("id",   DataType::integer, row.first));
      parameters.emplace_back(std::make_shared<AdoParameter>("name", DataType::varchar, row.second));

      ASSERT_TRUE(cmd.bind(parameters));
      EXPECT_EQ(cmd.execute(), 1);
      cmd.reset();
   }

   // ---------------------------------------------------------------------------------
   // check total rows with AdodbConnection executeScalar()
   EXPECT_EQ(conn.executeScalar("SELECT COUNT(*) FROM ##tbs_prepared_cmd_test"), "3");
   // check with AdodbCommand executeScalar()
   ASSERT_TRUE(cmd.prepare("SELECT COUNT(*) FROM ##tbs_prepared_cmd_test"));
   EXPECT_EQ(cmd.executeScalar(), "3");
   // ---------------------------------------------------------------------------------

   // prepare cmd for SELECT session
   ASSERT_TRUE(cmd.prepare("SELECT * FROM  ##tbs_prepared_cmd_test WHERE name = ?"));
   for (const std::string& value : { std::string("Budi"),std::string("Wati"),std::string("Yanto") } )
   {
      AdoParameterCollection parameters;
      parameters.emplace_back(std::make_shared<AdoParameter>("name", DataType::varchar, value));

      ASSERT_TRUE(cmd.bind(parameters));
      auto result = cmd.executeResult();
      ASSERT_TRUE(result != nullptr);
      ASSERT_EQ(result->totalRows(), 1);
      ASSERT_EQ(result->totalColumns(), 2);
      EXPECT_EQ(tbs::ComVariantHelper::toString(result->data().at(0).at(1)), value);

      cmd.reset();
   }

   // ADO allow calling repeated execute*() without re-bind() and reset()
   // ---------------------------------------------------------------------------------
   cmd.reset();
   AdoParameterCollection parameters;
   parameters.emplace_back(std::make_shared<AdoParameter>("name", DataType::varchar, "Wati"));
   ASSERT_TRUE(cmd.bind(parameters));

   AdodbResult sqlres = cmd.executeSqlResult();

   sqlres.navigator().moveFirst();
   ASSERT_EQ(sqlres.columnNames().at(0), "id");
   ASSERT_EQ(sqlres.totalRows(), 1);
   ASSERT_EQ(sqlres.totalColumns(), 2);
   EXPECT_EQ(sqlres.getStringValue("name"), "Wati");
   EXPECT_EQ(sqlres.getStringValue("id"), "2");
   EXPECT_EQ(tbs::ComVariantHelper::toString(sqlres.getVariantValue("name")), "Wati");

   auto res1 = cmd.execute();
   ASSERT_EQ(res1, 0);

   auto res2 = cmd.executeScalar();
   ASSERT_EQ(res2, std::string("2"));

   auto res3 = cmd.executeResult();
   EXPECT_EQ( tbs::ComVariantHelper::toString( res3->data().at(0).at(1)), "Wati");

   auto res4 = cmd.execute();
   ASSERT_EQ(res4, 0);

   auto res5 = cmd.executeScalar();
   ASSERT_EQ(res5, std::string("2"));
   // ---------------------------------------------------------------------------------
}