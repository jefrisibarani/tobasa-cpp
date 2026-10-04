#include <gtest/gtest.h>

#include <chrono>

#include "tobasa/util_time.h"

TEST(TobasaUtilTimeTest, ReportsCurrentTimeInConsistentUnits)
{
   tbs::TimeUtil timeUtil;

   const auto seconds = timeUtil.unixTimeSeconds();
   const auto millis = timeUtil.unixTimeMiliSeconds();
   const auto systemSeconds = static_cast<long long>(timeUtil.time());

   EXPECT_GE(seconds, 0LL);
   EXPECT_GE(millis, seconds * 1000LL);
   EXPECT_EQ(systemSeconds, seconds);
}

TEST(TobasaUtilTimeTest, UtcAndLocalTimeAreValidDateStructures)
{
   tbs::TimeUtil timeUtil;

   const auto utc = timeUtil.utcTime();
   const auto local = timeUtil.localTime();

   EXPECT_GE(utc.tm_year + 1900, 2020);
   EXPECT_GE(local.tm_year + 1900, 2020);
   EXPECT_GE(utc.tm_mon, 0);
   EXPECT_LE(utc.tm_mon, 11);
   EXPECT_GE(local.tm_mon, 0);
   EXPECT_LE(local.tm_mon, 11);
}

TEST(TobasaUtilTimeTest, ConvertsToCurrentEpochTimestamp)
{
   tbs::TimeUtil timeUtil;

   const auto now = std::chrono::system_clock::now();
   const auto nowSeconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
   const auto currentSeconds = timeUtil.unixTimeSeconds();

   EXPECT_GE(currentSeconds, nowSeconds - 5LL);
   EXPECT_LE(currentSeconds, nowSeconds + 5LL);
}
