#include <gtest/gtest.h>

#include "tobasa/datetime.h"

TEST(TobasaDateTimeTest, ParsesAndFormatsDateTime)
{
   tbs::DateTime value(tbs::LocalTime{});

   ASSERT_TRUE(value.parse("2024-02-29 13:04:05"));

   EXPECT_FALSE(value.isNullDateTime());
   EXPECT_EQ(value.isoDateString(), "2024-02-29");
   EXPECT_EQ(value.isoTimeString(), "13:04:05");
   EXPECT_EQ(value.isoDateTimeString(), "2024-02-29 13:04:05");
   EXPECT_EQ(value.isoDateTimeString(true), "2024-02-29 13:04:05.000000000");
   EXPECT_EQ(value.format("{:%Y/%m/%d %H:%M}"), "2024/02/29 13:04");
}

TEST(TobasaDateTimeTest, ParsesTimeWithEpochDate)
{
   tbs::DateTime value(tbs::LocalTime{});

   ASSERT_TRUE(value.parseTime("05:23:42"));

   EXPECT_EQ(value.isoDateString(), "1970-01-01");
   EXPECT_EQ(value.isoTimeString(), "05:23:42");
}

TEST(TobasaDateTimeTest, InvalidInputSetsNullDateTime)
{
   tbs::DateTime value(tbs::LocalTime{});

   EXPECT_FALSE(value.parse("not a date"));
   EXPECT_TRUE(value.isNullDateTime());
   EXPECT_EQ(value.isoDateTimeString(), "null");
   EXPECT_EQ(value.format(), "null");
}

TEST(TobasaDateTimeTest, InitializesTimezoneAndFormatsUtc)
{
   ASSERT_TRUE(tbs::DateTime::initTimezoneData());

   EXPECT_FALSE(tbs::DateTime::getCurrentTimezone().empty());

   auto value = tbs::DateTime::now();

   EXPECT_FALSE(value.isNullDateTime());
   EXPECT_EQ(value.isoDateTimeStringUTC().size(), 19U);
}

TEST(TobasaDateTimeTest, ConstructsFromSystemClockTimePoint)
{
   ASSERT_TRUE(tbs::DateTime::initTimezoneData());

   constexpr long long utcMilliseconds = 1685409828477LL;
   const std::chrono::system_clock::time_point timePoint{
      std::chrono::milliseconds{utcMilliseconds}};
   tbs::DateTime value(timePoint);

   EXPECT_EQ(value.toUnixTimeMiliSeconds(), utcMilliseconds);
   EXPECT_EQ(value.toUnixTimeSeconds(), utcMilliseconds / 1000);
   EXPECT_EQ(value.isoDateTimeStringUTC(), "2023-05-30 01:23:48");
   EXPECT_EQ(value.isoDateTimeStringUTC(true), "2023-05-30 01:23:48.477000000");
}

TEST(TobasaDateTimeTest, SupportsTimePointExampleOperations)
{
   ASSERT_TRUE(tbs::DateTime::initTimezoneData());

   auto expiredTime = tbs::DateTime();
   expiredTime.timePoint() += std::chrono::minutes{60};

   tbs::DateTime expiredTime2(expiredTime.timePoint());
   EXPECT_FALSE(expiredTime2.timePoint() > expiredTime.timePoint());

   expiredTime2.timePoint() += tbsdate::years{2};
   EXPECT_TRUE(expiredTime2.timePoint() > expiredTime.timePoint());

   const auto interval = expiredTime2.timePoint() - expiredTime.timePoint();
   EXPECT_EQ(interval.count(), std::chrono::duration_cast<std::chrono::nanoseconds>(tbsdate::years{2}).count());
}