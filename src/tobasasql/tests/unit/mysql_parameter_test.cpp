#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "tobasasql/mysql_common.h"

namespace {

using tbs::sql::DataType;
using tbs::sql::MysqlParameter;
using tbs::sql::ParameterDirection;

TEST(MysqlParameterTest, ConvertsCStringAndKeepsMetadata)
{
   MysqlParameter parameter("username", DataType::varchar, "alice", 64, 0, false, ParameterDirection::inputOutput);

   EXPECT_EQ(parameter.name(), "username");
   EXPECT_EQ(parameter.type(), DataType::varchar);
   EXPECT_EQ(parameter.size(), 64U);
   EXPECT_EQ(parameter.direction(), ParameterDirection::inputOutput);
   ASSERT_TRUE(std::holds_alternative<std::string>(parameter.value()));
   EXPECT_EQ(std::get<std::string>(parameter.value()), "alice");
}

TEST(MysqlParameterTest, RejectsSignedValueOutsideSqlTypeRange)
{
   EXPECT_THROW(
      MysqlParameter("small", DataType::smallint, int32_t{32768}), std::out_of_range);
}

TEST(MysqlParameterTest, RejectsNegativeValueForUnsignedSqlType)
{
   EXPECT_THROW(
      MysqlParameter("count", DataType::integer, int32_t{-1}, 0, 0, true), std::out_of_range);
}

TEST(MysqlParameterTest, CopiesBinaryPointerUsingExplicitSize)
{
   const uint8_t bytes[] = {0x01, 0x02, 0xA0};

   MysqlParameter parameter("payload", DataType::varbinary, static_cast<const void*>(bytes), sizeof(bytes));

   ASSERT_TRUE(std::holds_alternative<std::vector<uint8_t>>(parameter.value()));

   EXPECT_EQ(std::get<std::vector<uint8_t>>(parameter.value()),  
      (std::vector<uint8_t>{0x01, 0x02, 0xA0}));
}

TEST(MysqlParameterTest, RequiresSizeForBinaryPointer)
{
   const uint8_t byte = 0x01;
   EXPECT_THROW(
      MysqlParameter("payload", DataType::varbinary, static_cast<const void*>(&byte), 0), 
         std::invalid_argument);
}

} // namespace