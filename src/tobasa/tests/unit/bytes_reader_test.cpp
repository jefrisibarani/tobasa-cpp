#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "tobasa/bytes_reader.h"

TEST(BytesReaderTest, TracksOpenStateAndDataSize)
{
   std::vector<unsigned char> payload = {'h', 'i', '!', 0x00, 0x7f};
   const nonstd::span<const unsigned char> view(payload.data(), payload.size());
   tbs::BytesReader reader(view);

   EXPECT_TRUE(reader.isOpen());
   EXPECT_EQ(reader.dataSize(), static_cast<std::streamsize>(payload.size()));
}

TEST(BytesReaderTest, ReadsSequentiallyIntoBuffer)
{
   std::vector<unsigned char> payload = {'a', 'b', 'c', 'd', 'e', 'f'};
   const nonstd::span<const unsigned char> view(payload.data(), payload.size());
   tbs::BytesReader reader(view);

   uint8_t buffer[3] = {0};
   const auto firstRead = reader.read(buffer, 3);
   ASSERT_EQ(firstRead, 3);
   EXPECT_EQ(std::string(reinterpret_cast<const char*>(buffer), 3), "abc");

   uint8_t secondBuffer[4] = {0};
   const auto secondRead = reader.read(secondBuffer, 4);
   ASSERT_EQ(secondRead, 3);
   EXPECT_EQ(std::string(reinterpret_cast<const char*>(secondBuffer), 3), "def");
}

TEST(BytesReaderTest, ReadsIntoOutputStream)
{
   std::vector<unsigned char> payload = {'A', 'B', 'C', 'D'};
   const nonstd::span<const unsigned char> view(payload.data(), payload.size());
   tbs::BytesReader reader(view);
   std::ostringstream output;

   const auto firstRead = reader.read(output, 2);
   ASSERT_EQ(firstRead, 2);
   EXPECT_EQ(output.str(), "AB");

   std::ostringstream tail;
   const auto tailRead = reader.read(tail, 10);
   ASSERT_EQ(tailRead, 2);
   EXPECT_EQ(tail.str(), "CD");
}

TEST(BytesReaderTest, ReadsAtSpecificPositions)
{
   std::vector<unsigned char> payload = {'0', '1', '2', '3', '4', '5', '6', '7'};
   const nonstd::span<const unsigned char> view(payload.data(), payload.size());
   tbs::BytesReader reader(view);

   uint8_t buffer[3] = {0};
   const auto posRead = reader.readAt(2, buffer, 3);
   ASSERT_EQ(posRead, 3);
   EXPECT_EQ(std::string(reinterpret_cast<const char*>(buffer), 3), "234");

   std::ostringstream output;
   const auto streamRead = reader.readAt(5, output, 3);
   ASSERT_EQ(streamRead, 3);
   EXPECT_EQ(output.str(), "567");
}
