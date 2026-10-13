#include <gtest/gtest.h>

#include <tobasasql/database_connector.h>
#include <tobasasql/db_api.h>

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

TEST(DbApiUnitTest, DirectConnectionDoesNotRequirePool)
{
   DbConnection connection(makeSqliteOptions());
   ASSERT_TRUE(connection);
   EXPECT_EQ(connection.createCommand("SELECT 1").executeScalar(), "1");
   DbCommand command(connection, "SELECT 2");
   EXPECT_EQ(command.executeScalar(), "2");
   EXPECT_EQ(connection.backendType(), BackendType::sqlite);
   EXPECT_EQ(connection.status(), ConnectionStatus::ok);
   EXPECT_TRUE(connection.startTransaction());
   EXPECT_EQ(connection.execute("CREATE TABLE transaction_probe (id INTEGER)"), 0);
   EXPECT_TRUE(connection.rollbackTransaction());
   EXPECT_EQ(connection.executeScalar("SELECT COUNT(*) FROM sqlite_master WHERE name = 'transaction_probe'"), "0");
}

TEST(DbApiUnitTest, DatabaseConnectorReportsTransactionState)
{
   conf::ConnectorOption options;
   options.environment = "production";
   options.production.dbDriver = BackendType::sqlite;
   options.production.connectionString = "Database=:memory:;OpenMemory=True;";
   options.development = options.production;
   options.securitySalt.clear();

   DatabaseConnector connector(options, "transaction_state_test");
   connector.initSqlDriver();
   ASSERT_TRUE(connector.connect());
   EXPECT_FALSE(connector.isTranscationActive());
   EXPECT_TRUE(connector.beginTransaction());
   EXPECT_TRUE(connector.isTranscationActive());
   EXPECT_TRUE(connector.rollbackTransaction());
   EXPECT_FALSE(connector.isTranscationActive());
   EXPECT_TRUE(connector.disconnect());
}

TEST(DbApiUnitTest, ResultExposesPortableMetadataAndValues)
{
   ASSERT_TRUE(DateTime::initTimezoneData());
   DbConnection connection(makeSqliteOptions());
   auto reusable = connection.createCommand("SELECT :value AS value");
   reusable.parameter(DbParameter{"value", DataType::integer, int32_t{3}});
   EXPECT_EQ(reusable.executeScalar(), "3");
   reusable.reset();
   reusable.addParam("value", DataType::integer, int32_t{9});
   EXPECT_EQ(reusable.executeScalar(), "9");

   auto result = connection.createCommand("SELECT 7 AS id, 'value' AS label, 1 AS truth, '2002-02-11 19:55:00' AS stamp").executeQuery();

   ASSERT_TRUE(result.isValid());
   EXPECT_EQ(result.totalColumns(), 4);
   EXPECT_EQ(result.columnNames(), (std::vector<std::string>{"id", "label", "truth", "stamp"}));
   EXPECT_EQ(result.columnNumber("label"), 1);
   EXPECT_EQ(result.sqlQuery(), "SELECT 7 AS id, 'value' AS label, 1 AS truth, '2002-02-11 19:55:00' AS stamp");
   result.moveFirst();
   EXPECT_EQ(result.getInt64("id"), 7);
   EXPECT_EQ(result.getInt32(0), 7);
   EXPECT_TRUE(result.getBool(2));
   EXPECT_FLOAT_EQ(result.getFloat(0), 7.0f);
   EXPECT_DOUBLE_EQ(result.getDouble(0), 7.0);
   EXPECT_EQ(result.getString("label"), "value");
   EXPECT_FALSE(result.isNull("label"));
   EXPECT_NO_THROW((void)result.getDateTime(3));
}

TEST(DbApiUnitTest, NativeTimeAndTimestampParametersKeepTheirText)
{
   DbConnection connection(makeSqliteOptions());
   ASSERT_EQ(connection.execute("CREATE TABLE time_values (time_value TIME, stamp_value TIMESTAMP)"), 0);

   auto insert = connection.createCommand("INSERT INTO time_values VALUES (?, ?)", ParameterStyle::native);
   insert.addParam("time", DataType::time, std::string("12:20:00"));
   insert.addParam("stamp", DataType::timestamp, std::string("2002-02-11 19:55:00"));
   ASSERT_EQ(insert.execute(), 1);
   insert.reset();
   insert.addParam("time", DataType::time, std::string("03:40:00"));
   insert.addParam("stamp", DataType::timestamp, std::string("2003-03-14 15:21:00"));
   ASSERT_EQ(insert.execute(), 1);

   EXPECT_EQ(connection.executeScalar("SELECT quote(time_value) FROM time_values WHERE rowid = 1"), "'12:20:00'");
   EXPECT_EQ(connection.executeScalar("SELECT quote(stamp_value) FROM time_values WHERE rowid = 1"), "'2002-02-11 19:55:00'");
   EXPECT_EQ(connection.executeScalar("SELECT quote(time_value) FROM time_values WHERE rowid = 2"), "'03:40:00'");
   EXPECT_EQ(connection.executeScalar("SELECT quote(stamp_value) FROM time_values WHERE rowid = 2"), "'2003-03-14 15:21:00'");
}

TEST(DbApiUnitTest, TableLoadsEditsAndSavesRows)
{
   DbConnection connection(makeSqliteOptions());
   ASSERT_EQ(connection.execute("CREATE TABLE people (id INTEGER PRIMARY KEY, name TEXT NOT NULL)"), 0);
   ASSERT_EQ(connection.execute("CREATE TABLE labels (id INTEGER PRIMARY KEY, label TEXT NOT NULL)"), 0);
   ASSERT_EQ(connection.execute("INSERT INTO people (name) VALUES ('Ada')"), 1);
   ASSERT_EQ(connection.execute("INSERT INTO labels (label) VALUES ('alpha'), ('beta')"), 2);

   DbTable table(connection, "people", {}, false);
   table.init();
   ASSERT_TRUE(table.isValid());
   ASSERT_EQ(table.totalRows(), 1);
   ASSERT_EQ(table.totalColumns(), 2);
   EXPECT_EQ(table.columnInfo(table.columnPosition("name")).getName(), "name");
   ASSERT_TRUE(table.setupLookup("name", "SELECT id, label FROM labels ORDER BY id"));
   auto lookup = table.lookup("name");
   ASSERT_EQ(lookup.size(), 2);
   EXPECT_EQ(lookup.keyAt(0), "1");
   EXPECT_EQ(lookup.valueForKey("2"), "beta");
   EXPECT_EQ(lookup.keyForValue("alpha"), "1");

   table.moveFirst();
   EXPECT_EQ(table.getString("name"), "Ada");
   table.setValue("name", std::string("Grace"));
   EXPECT_TRUE(table.save());
   EXPECT_EQ(connection.executeScalar("SELECT name FROM people"), "Grace");
}

} // namespace