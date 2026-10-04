#include <gtest/gtest.h>

#include "tobasasql/sqlite_connection.h"

namespace {

using namespace tbs;

TEST(SqliteConnectionTest, InMemoryDatabase)
{
   sql::SqliteConnection conn;
   ASSERT_TRUE( conn.connect("OpenMemory=True;") );
   ASSERT_EQ(   conn.status(), sql::ConnectionStatus::ok);

   EXPECT_EQ(   conn.execute("SELECT 1"), 0         );
   EXPECT_EQ(   conn.executeScalar("SELECT 1"), "1" );

   EXPECT_EQ(conn.execute( "CREATE TABLE first_table (id INT NOT NULL, name VARCHAR(64) NOT NULL)"), 0 );
   EXPECT_EQ(conn.execute( "CREATE TABLE second_table (id INT NOT NULL, name VARCHAR(64) NOT NULL)"), 0 );
   EXPECT_EQ(conn.execute( "CREATE TABLE third_table (id INT NOT NULL, name VARCHAR(64) NOT NULL)"), 0 );

   EXPECT_TRUE( conn.tableOrViewExists("first_table", true) );
   EXPECT_TRUE( conn.tableOrViewExists("second_table", true) );
   EXPECT_TRUE( conn.tableOrViewExists("third_table", true) );

   EXPECT_FALSE( conn.tableOrViewExists("third_table", false) );
   EXPECT_FALSE( conn.tableOrViewExists("third_table", false) );
   EXPECT_FALSE( conn.tableOrViewExists("third_table", false) );

   std::vector<std::string>tableNames;
   conn.getTablesOrViews(tableNames, true);
   EXPECT_EQ(tableNames.size(), 3);
}



} // namespace