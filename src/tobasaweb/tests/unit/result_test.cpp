#include <gtest/gtest.h>

#include "tobasaweb/result.h"

TEST(TobasaWebTest, StoresResultContentAndStatus)
{
   tbs::http::Result result("hello", "text/plain");
   result.statusCode(tbs::http::StatusCode::CREATED);

   EXPECT_EQ(result.content(), "hello");
   EXPECT_EQ(result.contentType(), "text/plain");
   EXPECT_EQ(result.httpStatus().code(), tbs::http::StatusCode::CREATED);
}