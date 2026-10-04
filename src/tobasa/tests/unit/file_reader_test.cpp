#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "tobasa/file_reader.h"

namespace fs = std::filesystem;

namespace {

fs::path makeTempFile(const std::string& content)
{
   const auto tempDir = fs::temp_directory_path() / "tobasa_file_reader_test";
   fs::create_directories(tempDir);

   const auto path = tempDir / "sample.bin";
   std::ofstream output(path, std::ios::binary | std::ios::trunc);
   output.write(content.data(), static_cast<std::streamsize>(content.size()));
   return path;
}

} // namespace

TEST(FileReaderTest, TracksOpenStateAndDataSize)
{
   const std::string payload = "hello world";
   const auto filePath = makeTempFile(payload);
   {
      tbs::FileReader reader(filePath.string());
      EXPECT_TRUE(reader.isOpen());
      EXPECT_EQ(reader.dataSize(), static_cast<std::streamsize>(payload.size()));
   }

   fs::remove(filePath);
   fs::remove(filePath.parent_path());
}

TEST(FileReaderTest, ReadsSequentiallyIntoBuffer)
{
   const std::string payload = "abcdefghi";
   const auto filePath = makeTempFile(payload);
   {
      tbs::FileReader reader(filePath.string());

      uint8_t buffer[4] = {0};
      const auto firstRead = reader.read(buffer, 4);
      ASSERT_EQ(firstRead, 4);
      EXPECT_EQ(std::string(reinterpret_cast<const char*>(buffer), 4), "abcd");

      uint8_t nextBuffer[5] = {0};
      const auto secondRead = reader.read(nextBuffer, 5);
      ASSERT_EQ(secondRead, 5);
      EXPECT_EQ(std::string(reinterpret_cast<const char*>(nextBuffer), 5), "efghi");
   }

   fs::remove(filePath);
   fs::remove(filePath.parent_path());
}

TEST(FileReaderTest, ReadsIntoOutputStream)
{
   const std::string payload = "abcdef";
   const auto filePath = makeTempFile(payload);
   {
      tbs::FileReader reader(filePath.string());
      std::ostringstream output;

      const auto firstRead = reader.read(output, 3);
      ASSERT_EQ(firstRead, 3);
      EXPECT_EQ(output.str(), "abc");

      std::ostringstream tail;
      const auto secondRead = reader.read(tail, 10);
      ASSERT_EQ(secondRead, 3);
      EXPECT_EQ(tail.str(), "def");
   }

   fs::remove(filePath);
   fs::remove(filePath.parent_path());
}

TEST(FileReaderTest, ReadsAtSpecificPositions)
{
   const std::string payload = "abcdefghij";
   const auto filePath = makeTempFile(payload);
   {
      tbs::FileReader reader(filePath.string());

      uint8_t buffer[4] = {0};
      const auto atRead = reader.readAt(2, buffer, 4);
      ASSERT_EQ(atRead, 4);
      EXPECT_EQ(std::string(reinterpret_cast<const char*>(buffer), 4), "cdef");

      std::ostringstream output;
      const auto streamRead = reader.readAt(7, output, 3);
      ASSERT_EQ(streamRead, 3);
      EXPECT_EQ(output.str(), "hij");
   }

   fs::remove(filePath);
   fs::remove(filePath.parent_path());
}
