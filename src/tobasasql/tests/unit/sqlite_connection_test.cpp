#include <gtest/gtest.h>

#include <utility>

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


TEST(SqliteConnectionTest, MoveConstructionTransfersConnectionAndNotifier)
{
   sql::SqliteConnection source;
   ASSERT_TRUE(source.connect("OpenMemory=True;"));

   bool notified = false;
   source.notificationHandler = [&notified](const NotifyEventArgs&) { notified = true; };

   sql::SqliteConnection moved(std::move(source));

   EXPECT_EQ(source.status(), sql::ConnectionStatus::bad);
   EXPECT_EQ(moved.executeScalar("SELECT 1"), "1");
   moved.onNotifyInfo("move test");
   EXPECT_TRUE(notified);
}

TEST(SqliteConnectionTest, MoveAssignmentTransfersConnectionAndReleasesDestination)
{
   sql::SqliteConnection destination;
   ASSERT_TRUE(destination.connect("OpenMemory=True;"));
   EXPECT_EQ(destination.execute("CREATE TABLE old_connection_table (id INT)"), 0);

   sql::SqliteConnection source;
   ASSERT_TRUE(source.connect("OpenMemory=True;"));
   EXPECT_EQ(source.execute("CREATE TABLE moved_connection_table (id INT)"), 0);

   destination = std::move(source);

   EXPECT_EQ(source.status(), sql::ConnectionStatus::bad);
   EXPECT_TRUE(destination.tableOrViewExists("moved_connection_table", true));
   EXPECT_FALSE(destination.tableOrViewExists("old_connection_table", true));
}


} // namespace