#include <gtest/gtest.h>

#include "tobasasql/sql_util.h"
#include "tobasasql/sql_parameter_rewriter.h"

namespace {
using namespace tbs;
using namespace tbs::sql;

TEST(SqlParameterRewriterTest, UsesBackendSpecificPlaceholders)
{
   const std::string sql = "SELECT * FROM users WHERE id = :id AND name = :name";

   SqlParameterRewriter mysql(BackendType::mysql);
   EXPECT_EQ(mysql.rewrite(sql), "SELECT * FROM users WHERE id = ? AND name = ?");
   EXPECT_EQ(mysql.parameters(), (std::vector<std::string>{"id", "name"}));

   SqlParameterRewriter postgres(BackendType::pgsql);
   EXPECT_EQ(postgres.rewrite(sql), "SELECT * FROM users WHERE id = $1 AND name = $2");
   EXPECT_EQ(postgres.parameters(), (std::vector<std::string>{"id", "name"}));
}

TEST(SqlParameterRewriterTest, PreservesStringsCommentsAndCasts)
{
   SqlParameterRewriter rewriter(BackendType::pgsql);
   const std::string sql =
      "SELECT 'literal :ignored', value::text FROM data "
      "WHERE id = :id /* :block_ignored */ -- :line_ignored\n"
      "AND note = 'it''s :also_ignored'";

   EXPECT_EQ(rewriter.rewrite(sql),
      "SELECT 'literal :ignored', value::text FROM data "
      "WHERE id = $1 /* :block_ignored */ -- :line_ignored\n"
      "AND note = 'it''s :also_ignored'");
   EXPECT_EQ(rewriter.parameters(), (std::vector<std::string>{"id"}));
}

TEST(SqlParameterRewriterTest, ClearsParametersOnEachRewrite)
{
   SqlParameterRewriter rewriter(BackendType::sqlite);

   EXPECT_EQ(rewriter.rewrite("SELECT :first"), "SELECT ?");
   EXPECT_EQ(rewriter.parameters(), (std::vector<std::string>{"first"}));

   EXPECT_EQ(rewriter.rewrite("SELECT 1"), "SELECT 1");
   EXPECT_TRUE(rewriter.parameters().empty());
}

TEST(SqlParameterRewriterTest, PreservesNativePostgresPlaceholders)
{
   const std::string sql = "SELECT * FROM users WHERE id = $1 AND name = :name";
   const std::vector<std::string> params = { "name" };

   EXPECT_EQ(expandNamedParams(sql, ParameterStyle::named,  params, BackendType::pgsql), sql);
   EXPECT_EQ(expandNamedParams(sql, ParameterStyle::native, params, BackendType::pgsql), sql);
}

} // namespace