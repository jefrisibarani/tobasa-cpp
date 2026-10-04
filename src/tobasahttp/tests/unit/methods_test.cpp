#include <gtest/gtest.h>

#include "tobasahttp/methods.h"

TEST(TobasaHttpTest, ConvertsHttpMethods)
{
   EXPECT_EQ(tbs::http::httpMethodToString(tbs::http::Method::GET), "GET");
   EXPECT_EQ(tbs::http::httpMethodFromString("DELETE"), tbs::http::Method::DEL);
   EXPECT_EQ(tbs::http::httpMethodFromString("invalid"), tbs::http::Method::UNKNOWNN);
}