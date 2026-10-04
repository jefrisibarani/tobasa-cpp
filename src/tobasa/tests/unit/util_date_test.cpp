#include <gtest/gtest.h>

#include <chrono>
#include <ctime>

#include "tobasa/util_date.h"

TEST(TobasaUtilDateTest, ParsesSysSecondsAndFormatsIt)
{
   tbsdate::sys_seconds value{};

   ASSERT_TRUE(tbs::util::parseDate(value, "2024-02-29 13:04:05", "%Y-%m-%d %H:%M:%S"));
   EXPECT_EQ(tbs::util::formatDate(value, "{:%Y-%m-%d}"), "2024-02-29");
}


TEST(TobasaUtilDateTest, ParsesTmAndReadsFields)
{
   std::tm value{};

   ASSERT_TRUE(tbs::util::parseDate(value, "2024-02-29", "%Y-%m-%d"));
   EXPECT_EQ(value.tm_year + 1900, 2024);
   EXPECT_EQ(value.tm_mon + 1, 2);
   EXPECT_EQ(value.tm_mday, 29);
}

TEST(TobasaUtilDateTest, RejectsInvalidDateString)
{
   tbsdate::sys_seconds value{};

   EXPECT_FALSE(tbs::util::parseDate(value, "not-a-date", "%Y-%m-%d %H:%M:%S"));
}

TEST(TobasaUtilDateTest, FormatsChronoTimePoint)
{
   const auto tp = std::chrono::system_clock::time_point{
      std::chrono::seconds{1700000000}};

   EXPECT_EQ(tbs::util::formatDate("{:%Y-%m-%d}", tp), "2023-11-14");
}

TEST(TobasaUtilDateTest, FormatDateNowReturnsNonEmptyString)
{
   const auto current = tbs::util::formatDateNow("{:%Y-%m-%d}");
   EXPECT_FALSE(current.empty());
   EXPECT_EQ(current.size(), 10U);
}

TEST(TobasaUtilDateTest, InvalidFormatThrowsRuntimeError)
{
   const auto tp = std::chrono::system_clock::now();

   EXPECT_THROW(tbs::util::formatDate("", tp), std::runtime_error);
}
