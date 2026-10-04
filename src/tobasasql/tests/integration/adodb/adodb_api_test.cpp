#include <gtest/gtest.h>

#include <string>

#include "tobasa_test_support/environment.h"

#include "tobasasql/com_initializer.h"
#include "tobasasql/adodb_connection.h"
#include "tobasasql/adodb_result.h"
#include "tobasasql/adodb_command.h"

using namespace tbs;
using namespace tbs::sql;

TEST(AdodbInnerAPIIntegrationTest, ConnectExecuteAndExecuteScalar)
{
   const std::string connectionString = test_support::loadEnvironmentValue(
      "TOBASA_ADODB_TEST_CONNECTION_SQLNCLI",
      std::string(TOBASA_PROJECT_SOURCE_DIR) + "/.env");

   if (connectionString.empty())
      GTEST_SKIP() << "Set TOBASA_ADODB_TEST_CONNECTION_SQLNCLI or add it to .env";

   ComInitializer comInitializer(true);


   AdodbConnection conn;
   ASSERT_TRUE( conn.connect(connectionString)      );
   EXPECT_EQ(   conn.execute("SELECT 1"), 0         );
   EXPECT_EQ(   conn.executeScalar("SELECT 1"), "1" );



   AdodbCommand cmd(&conn);

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



   AdodbResult  res(&conn);
   ASSERT_TRUE( res.runQuery("SELECT 1")    );
   EXPECT_EQ(   res.getStringValue(0), "1"  );
   
}