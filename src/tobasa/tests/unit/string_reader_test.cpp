#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "tobasa/string_reader.h"

TEST(StringReaderTest, TracksOpenStateAndDataSize)
{
   const std::string payload = "hello world";
   tbs::StringReader reader(payload);

   EXPECT_TRUE(reader.isOpen());
   EXPECT_EQ(reader.dataSize(), static_cast<std::streamsize>(payload.size()));
}

TEST(StringReaderTest, ReadsSequentiallyIntoBuffer)
{
   const std::string payload = "abcdefghij";
   tbs::StringReader reader(payload);

   uint8_t buffer[5] = {0};
   const auto bytesRead = reader.read(buffer, 5);

   ASSERT_EQ(bytesRead, 5);
   EXPECT_EQ(std::string(reinterpret_cast<const char*>(buffer), 5), "abcde");

   uint8_t nextBuffer[6] = {0};
   const auto nextBytes = reader.read(nextBuffer, 6);
   ASSERT_EQ(nextBytes, 5);
   EXPECT_EQ(std::string(reinterpret_cast<const char*>(nextBuffer), 5), "fghij");
}

TEST(StringReaderTest, ReadsIntoOutputStream)
{
   const std::string payload = "abcdef";
   tbs::StringReader reader(payload);
   std::ostringstream output;

   const auto bytesRead = reader.read(output, 3);

   ASSERT_EQ(bytesRead, 3);
   EXPECT_EQ(output.str(), "abc");

   std::ostringstream remainder;
   const auto moreBytes = reader.read(remainder, 10);
   ASSERT_EQ(moreBytes, 3);
   EXPECT_EQ(remainder.str(), "def");
}

TEST(StringReaderTest, ReadsAtSpecificPositions)
{
   const std::string payload = "abcdefghi";
   tbs::StringReader reader(payload);

   uint8_t buffer[4] = {0};
   const auto bytesRead = reader.readAt(2, buffer, 4);

   ASSERT_EQ(bytesRead, 4);
   EXPECT_EQ(std::string(reinterpret_cast<const char*>(buffer), 4), "cdef");

   std::ostringstream output;
   const auto streamBytes = reader.readAt(6, output, 3);
   ASSERT_EQ(streamBytes, 3);
   EXPECT_EQ(output.str(), "ghi");
}
