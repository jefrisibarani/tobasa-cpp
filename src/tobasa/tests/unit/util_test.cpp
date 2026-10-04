#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "tobasa/util.h"

TEST(TobasaUtilTest, FindsElementsInVectorAndFormatsCsv)
{
   const std::vector<int> values{10, 20, 30};

   EXPECT_EQ(tbs::util::findPositionInVector(values, 20), 1L);
   EXPECT_EQ(tbs::util::findPositionInVector(values, 99), tbs::NOT_FOUND);
   EXPECT_EQ(tbs::util::vetorToCsvString(values), "10, 20, 30");
}

TEST(TobasaUtilTest, GeneratesRandomAndUniqueIdentifiers)
{
   const auto uniqueId = tbs::util::generateUniqueId();
   EXPECT_FALSE(uniqueId.empty());
   EXPECT_EQ(uniqueId.size(), 20U);

   const auto randomString = tbs::util::getRandomString(16);
   EXPECT_EQ(randomString.size(), 16U);
   EXPECT_TRUE(std::all_of(randomString.begin(), randomString.end(), [](char c) {
      return std::isdigit(static_cast<unsigned char>(c)) ||
             std::isupper(static_cast<unsigned char>(c)) ||
             std::islower(static_cast<unsigned char>(c));
   }));

   const auto randomNumber = tbs::util::getRandomNumber(8);
   EXPECT_EQ(randomNumber.size(), 8U);
   EXPECT_TRUE(std::all_of(randomNumber.begin(), randomNumber.end(), [](char c) {
      return std::isdigit(static_cast<unsigned char>(c));
   }));
}

TEST(TobasaUtilTest, ConvertsThreadIdAndMillisecondsToStrings)
{
   const auto currentThreadId = std::this_thread::get_id();
   std::stringstream expected;
   expected << currentThreadId;

   EXPECT_EQ(tbs::util::threadId(currentThreadId), expected.str());
   EXPECT_EQ(tbs::util::readMilliseconds(1234567LL), "20 m 34 s 567 ms");
   EXPECT_EQ(tbs::util::readMilliseconds(3600000LL), "1 h ");
   EXPECT_EQ(tbs::util::readMilliseconds(90061LL), "1 m 30 s 61 ms");
}

TEST(TobasaUtilTest, ParsesByteSizesFromText)
{
   EXPECT_EQ(tbs::util::parseSizeInBytes("10B"), 10LL);
   EXPECT_EQ(tbs::util::parseSizeInBytes("2KB"), 2048LL);
   EXPECT_EQ(tbs::util::parseSizeInBytes("2mb"), 2LL * 1024LL * 1024LL);
   EXPECT_EQ(tbs::util::parseSizeInBytes("1GB"), 1024LL * 1024LL * 1024LL);
   EXPECT_EQ(tbs::util::parseSizeInBytes("abc"), -1LL);
   EXPECT_EQ(tbs::util::parseSizeInBytes("0KB"), -1LL);
}
