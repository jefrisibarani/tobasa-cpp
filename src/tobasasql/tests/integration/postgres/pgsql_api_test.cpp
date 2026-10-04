#include <gtest/gtest.h>

#include <string>

#include "tobasa_test_support/environment.h"
#include "tobasasql/pgsql_connection.h"
#include "tobasasql/pgsql_result.h"
#include "tobasasql/pgsql_command.h"



using namespace tbs;
using namespace tbs::sql;

TEST(PgsqlInnerAPIIntegration_Test, ConnectExecuteAndExecuteScalar)
{
   const std::string connectionString = tbs::test_support::loadEnvironmentValue(
      "TOBASA_PGSQL_TEST_CONNECTION",
      std::string(TOBASA_PROJECT_SOURCE_DIR) + "/.env");

   if (connectionString.empty())
      GTEST_SKIP() << "Set TOBASA_PGSQL_TEST_CONNECTION or add it to .env";

   PgsqlConnection conn;
   ASSERT_TRUE( conn.connect(connectionString)      );
   EXPECT_EQ(   conn.execute("SELECT 1"), 0         );
   EXPECT_EQ(   conn.executeScalar("SELECT 1"), "1" );

   PgsqlCommand cmd(&conn);

   ASSERT_TRUE( cmd.query("SELECT 1", {}) );
   EXPECT_EQ(   cmd.execute(), 0          );
                cmd.reset();

   EXPECT_EQ(   cmd.executeScalar(), "1"  );
                cmd.reset();

   // with prepare() always bind() then execute*()
   ASSERT_TRUE( cmd.prepare("SELECT 1")   );
   ASSERT_TRUE( cmd.bind({})              ); // bind with empty parameter
   EXPECT_EQ(   cmd.execute(), 0);
                cmd.reset();

   EXPECT_EQ(   cmd.executeScalar(), "1"  );
                cmd.reset(); 



   PgsqlResult  res(&conn);
   ASSERT_TRUE( res.runQuery("SELECT 1")    );
   EXPECT_EQ(   res.getStringValue(0), "1"  );
   
}