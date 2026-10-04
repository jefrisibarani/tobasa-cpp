#include <gtest/gtest.h>

#include "tobasa/util_string.h"

TEST(TobasaUtilStringTest, ConvertsCase)
{
   EXPECT_EQ(tbs::util::toLower("HELLO"), "hello");
   EXPECT_EQ(tbs::util::toUpper("hello"), "HELLO");

   std::string value = "MiXeD";
   tbs::util::strLower(value);
   EXPECT_EQ(value, "mixed");

   tbs::util::strUpper(value);
   EXPECT_EQ(value, "MIXED");
}

TEST(TobasaUtilStringTest, TrimsAndReplacesText)
{
   EXPECT_EQ(tbs::util::trim(std::string{"  hello world\n\t "}), "hello world");
   EXPECT_EQ(tbs::util::replace("hello world", "world", "there"), "hello there");

   std::string value = "a,b,a";
   tbs::util::replaceAll(value, "a", "x");
   EXPECT_EQ(value, "x,b,x");
}

TEST(TobasaUtilStringTest, ChecksPrefixSuffixAndContains)
{
   EXPECT_TRUE(tbs::util::startsWith(std::string{"hello world"}, std::string{"hello"}));
   EXPECT_TRUE(tbs::util::endsWith(std::string_view{"hello world"}, std::string_view{"world"}));
   EXPECT_TRUE(tbs::util::contains(std::string{"hello world"}, std::string{"world"}));
   EXPECT_FALSE(tbs::util::contains(std::string{"hello world"}, std::string{"moon"}));
}

TEST(TobasaUtilStringTest, SplitsStringByDelimiter)
{
   const auto parts = tbs::util::split("alpha,beta,gamma", ',');
   ASSERT_EQ(parts.size(), 3U);
   EXPECT_EQ(parts[0], "alpha");
   EXPECT_EQ(parts[1], "beta");
   EXPECT_EQ(parts[2], "gamma");

   const auto withDelim = tbs::util::split("one::two::three", "::");
   ASSERT_EQ(withDelim.size(), 3U);
   EXPECT_EQ(withDelim[0], "one");
   EXPECT_EQ(withDelim[1], "two");
   EXPECT_EQ(withDelim[2], "three");
}

TEST(TobasaUtilStringTest, DetectsNumericAndBooleanValues)
{
   EXPECT_TRUE(tbs::util::isNumber("12345"));
   EXPECT_FALSE(tbs::util::isNumber("12a45"));

   EXPECT_TRUE(tbs::util::strToBool("true"));
   EXPECT_TRUE(tbs::util::strToBool("Y"));
   EXPECT_TRUE(tbs::util::strToBool("1"));
   EXPECT_FALSE(tbs::util::strToBool("false"));

   EXPECT_EQ(tbs::util::boolToStr(true), "true");
   EXPECT_EQ(tbs::util::boolToStr(false), "false");
}

TEST(TobasaUtilStringTest, RemovesWhitespaceAndQuotes)
{
   std::string value = "  abc def  ";
   EXPECT_TRUE(tbs::util::removeTrailingWhiteSpace(value));
   EXPECT_EQ(value, "  abc def");

   auto quoted = tbs::util::trimDoubleQuote("\"hello\"");
   EXPECT_EQ(quoted, "hello");

   std::string clean = "a b c";
   EXPECT_TRUE(tbs::util::removeWhiteSpace(clean));
   EXPECT_EQ(clean, "abc");
}
