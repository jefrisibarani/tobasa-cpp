#include <gtest/gtest.h>

#include "tobasasql/mysql_command.h"

namespace {

using namespace tbs;   

TEST(MysqlCommandBasicTest, NullConnectionThrow)
{
   sql::MysqlCommand cmd(nullptr);
   EXPECT_THROW( cmd.query("SELECT 1", {}),  SqlException);
   EXPECT_THROW( cmd.prepare("SELECT 1"),    SqlException);
   EXPECT_THROW( cmd.bind({}),               SqlException);
   EXPECT_THROW( cmd.reset(),                SqlException);
}

TEST(MysqlCommandBasicTest, CloseSafeForNullConnection)
{
   sql::MysqlCommand cmd(nullptr);
   EXPECT_NO_THROW(  cmd.close());
}

} // namespace