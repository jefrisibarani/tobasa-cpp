#include <gtest/gtest.h>

#include "tobasa/uuid.h"

TEST(TobasaTest, GeneratesUuid)
{
   const std::string value = tbs::uuid::generate();
   EXPECT_EQ(value.size(), 36U);
   EXPECT_EQ(value[8], '-');
   EXPECT_EQ(value[13], '-');
   EXPECT_EQ(value[18], '-');
   EXPECT_EQ(value[23], '-');
}