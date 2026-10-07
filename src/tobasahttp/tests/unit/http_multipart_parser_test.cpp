#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

#include "tobasa/span.h"
#include "tobasahttp/multipart_body_reader.h"
#include "tobasahttp/multipart_parser.h"

namespace {

namespace fs = std::filesystem;

class TemporaryDirectory
{
private:
   fs::path _path;

public:
   TemporaryDirectory()
      : _path{fs::temp_directory_path() / ("tobasa-multipart-test-" + std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()))}
   {}

   ~TemporaryDirectory()
   {
      std::error_code error;
      fs::remove_all(_path, error);
   }

   const fs::path& path() const { return _path; }
};

}

TEST(TobasaHttpMultipartParserTest, RemovesTemporaryFileWhenMultipartParseFails)
{
   TemporaryDirectory tempDir;
   const std::string boundary = "cleanup-test-boundary";
   const std::string body =
      "--" + boundary + "\r\n"
      "Content-Disposition: form-data; name=\"file\"; filename=\"payload.bin\"\r\n"
      "Content-Type: application/octet-stream\r\n\r\n"
      "partial-payload\r\n--" + boundary + "\r\n";

   tbs::http::parser::MultipartParser parser(tempDir.path().string());
   ASSERT_TRUE(parser.applyBoundary(boundary));
   parser.contentLength(body.size());

   auto info = parser.parse(reinterpret_cast<const uint8_t *>(body.data()), body.size());

   EXPECT_FALSE(info.success());
   ASSERT_TRUE(fs::exists(tempDir.path()));
   EXPECT_EQ(fs::directory_iterator(tempDir.path()), fs::directory_iterator{});
}

TEST(TobasaHttpMultipartParserTest, KeepsCompletedFileUntilMultipartBodyCleanup)
{
   TemporaryDirectory tempDir;
   const std::string boundary = "successful-upload-boundary";
   const std::string fileContents = "multipart file payload";
   const std::string body =
      "--" + boundary + "\r\n"
      "Content-Disposition: form-data; name=\"file\"; filename=\"payload.bin\"\r\n"
      "Content-Type: application/octet-stream\r\n\r\n" +
      fileContents + "\r\n--" + boundary + "--\r\n";

   tbs::http::parser::MultipartParser parser(tempDir.path().string());
   ASSERT_TRUE(parser.applyBoundary(boundary));
   parser.contentLength(body.size());

   auto info = parser.parse(reinterpret_cast<const uint8_t *>(body.data()), body.size());

   ASSERT_TRUE(info.success()) << info.message();
   ASSERT_TRUE(parser.done());
   auto multipartBody = parser.multipartBody();
   ASSERT_NE(multipartBody, nullptr);

   auto part = multipartBody->find("file");
   ASSERT_NE(part, nullptr);
   ASSERT_TRUE(part->isFile);
   ASSERT_TRUE(fs::exists(part->location));

   std::ifstream file(part->location, std::ios::binary);
   ASSERT_TRUE(file.is_open());
   const std::string actualContents{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
   file.close();
   EXPECT_EQ(actualContents, fileContents);

   const auto filePath = part->location;
   multipartBody->cleanup(true);
   EXPECT_FALSE(fs::exists(filePath));
}

TEST(TobasaHttpMultipartParserTest, ParsesFileAcrossInputFragments)
{
   TemporaryDirectory tempDir;
   const std::string boundary = "fragmented-upload-boundary";
   const std::string fileContents = "fragmented file payload across chunks";
   const std::string body =
      "--" + boundary + "\r\n"
      "Content-Disposition: form-data; name=\"file\"; filename=\"payload.bin\"\r\n"
      "Content-Type: application/octet-stream\r\n\r\n" +
      fileContents + "\r\n--" + boundary + "--\r\n";

   tbs::http::parser::MultipartParser parser(tempDir.path().string());
   ASSERT_TRUE(parser.applyBoundary(boundary));
   parser.contentLength(body.size());

   constexpr size_t fragmentSize = 7;
   for (size_t offset = 0; offset < body.size(); offset += fragmentSize)
   {
      const size_t length = std::min(fragmentSize, body.size() - offset);
      auto info = parser.parse(reinterpret_cast<const uint8_t *>(body.data() + offset), length);
      ASSERT_TRUE(info.success()) << "offset " << offset << ": " << info.message();
      if (offset + length < body.size())
         EXPECT_FALSE(parser.done()) << "offset " << offset;
   }

   ASSERT_TRUE(parser.done());
   auto multipartBody = parser.multipartBody();
   ASSERT_NE(multipartBody, nullptr);
   auto part = multipartBody->find("file");
   ASSERT_NE(part, nullptr);
   ASSERT_TRUE(fs::exists(part->location));

   std::ifstream file(part->location, std::ios::binary);
   ASSERT_TRUE(file.is_open());
   const std::string actualContents{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
   file.close();
   EXPECT_EQ(actualContents, fileContents);

   const auto filePath = part->location;
   multipartBody->cleanup(true);
   EXPECT_FALSE(fs::exists(filePath));
}

TEST(TobasaHttpMultipartParserTest, FeedWithoutHandlerIsDeferredNotRejected)
{
   auto reader = std::make_shared<tbs::http::MultipartBodyReader>(
      []() {},
      tbs::span<const char>{""},
      0,
      0);

   const std::string body = "hello";
   auto info = reader->feed(tbs::span<const char>(body.data(), body.size()), body.size());

   EXPECT_FALSE(info.success());
   EXPECT_EQ(info.message(), "multipart body handler not ready");
}
