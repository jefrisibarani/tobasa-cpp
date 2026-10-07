#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

#include <tobasa/datetime.h>
#include "tobasahttp/request.h"
#include "tobasahttp/response.h"

namespace {

namespace fs = std::filesystem;
using namespace tbs::http;

class TemporaryDirectory
{
private:
   fs::path _path;

public:
   TemporaryDirectory()
      : _path{fs::temp_directory_path() / ("tobasa-http-range-test-" + std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()))}
   {
      fs::create_directories(_path);
   }

   ~TemporaryDirectory()
   {
      std::error_code error;
      fs::remove_all(_path, error);
   }

   fs::path writeFile(const std::string& name, const std::string& content) const
   {
      auto filePath = _path / name;
      std::ofstream file(filePath, std::ios::binary);
      file.write(content.data(), static_cast<std::streamsize>(content.size()));
      return filePath;
   }
};

std::shared_ptr<Request> makeRequest(const std::string& method = "GET")
{
   auto request = std::make_shared<Request>(HttpVersion::one);
   request->method(method);
   return request;
}

std::shared_ptr<Response> makeFileResponse(const fs::path& filePath, bool enableFileRanges = false)
{
   auto response = std::make_shared<Response>(HttpVersion::one);
   response->fileContent(filePath.string());
   response->enableFileRangeResponse(enableFileRanges);
   return response;
}

std::string takeBuffer(asio::streambuf& buffer)
{
   std::istream input(&buffer);
   return {std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
}

std::string serializeHttp1(const std::shared_ptr<Response>& response, size_t bufferSize = 3)
{
   static const bool timezoneInitialized = tbs::DateTime::initTimezoneData();
   if (!timezoneInitialized)
      throw std::runtime_error("Failed to initialize timezone data for response tests");

   ResponseSerializer serializer(response);
   asio::streambuf buffer;
   std::string output;
   for (;;)
   {
      const auto bytesWritten = serializer.serializeHttp1(&buffer, bufferSize);
      output += takeBuffer(buffer);
      if (serializer.readLeft() == 0 &&
          (!response->useChunkedEncoding() || bytesWritten == 5))
         break;
      if (bytesWritten == 0)
         break;
   }
   return output;
}

std::string serializeHttp2Body(ResponseSerializer& serializer, size_t bufferSize = 3)
{
   std::string output;
#ifdef TOBASA_HTTP2_WRITE_RESPONSE_NO_COPY_DATA
   asio::streambuf buffer;
   while (serializer.readLeft() > 0)
   {
      serializer.serializeHttp2(&buffer, bufferSize);
      output += takeBuffer(buffer);
   }
#else
   std::vector<uint8_t> buffer(bufferSize);
   while (serializer.readLeft() > 0)
   {
      uint32_t dataFlags = 0;
      const auto bytesRead = serializer.serializeHttp2(buffer.data(), buffer.size(), &dataFlags);
      if (bytesRead <= 0)
         break;
      output.append(reinterpret_cast<const char*>(buffer.data()), static_cast<size_t>(bytesRead));
   }
#endif
   return output;
}

std::string responseBody(const std::string& wireResponse)
{
   const auto bodyStart = wireResponse.find("\r\n\r\n");
   if (bodyStart == std::string::npos)
      return {};
   return wireResponse.substr(bodyStart + 4);
}

} // namespace

TEST(TobasaHttpRangeResponseTest, ServesClosedOpenAndSuffixRanges)
{
   TemporaryDirectory tempDir;
   const auto filePath = tempDir.writeFile("payload.bin", "0123456789");

   struct TestCase
   {
      std::string range;
      std::string contentRange;
      std::string content;
   };

   const std::vector<TestCase> cases = {
      {"bytes=2-5", "bytes 2-5/10", "2345"},
      {"bytes=7-", "bytes 7-9/10", "789"},
      {"bytes=-3", "bytes 7-9/10", "789"}
   };

   for (const auto& testCase : cases)
   {
      auto request = makeRequest();
      request->setHeader("Range", testCase.range);
      auto response = makeFileResponse(filePath, true);

      response->prepareFileRangeResponse(*request);

      EXPECT_EQ(response->statusCode(), StatusCode::PARTIAL_CONTENT);
      EXPECT_EQ(response->headers().value("Accept-Ranges"), "bytes");
      EXPECT_EQ(response->headers().value("Content-Range"), testCase.contentRange);
      EXPECT_EQ(response->contentSize(), testCase.content.size());

      const auto wireResponse = serializeHttp1(response);
      EXPECT_EQ(responseBody(wireResponse), testCase.content);
      EXPECT_NE(wireResponse.find("Content-Length: " + std::to_string(testCase.content.size())), std::string::npos);
   }
}

TEST(TobasaHttpRangeResponseTest, DoesNotAdvertiseOrApplyRangesByDefault)
{
   TemporaryDirectory tempDir;
   const auto filePath = tempDir.writeFile("payload.bin", "0123456789");
   auto request = makeRequest();
   request->setHeader("Range", "bytes=2-5");
   auto response = makeFileResponse(filePath);

   response->prepareFileRangeResponse(*request);

   EXPECT_EQ(response->statusCode(), StatusCode::OK);
   EXPECT_TRUE(response->headers().value("Accept-Ranges").empty());
   EXPECT_EQ(response->contentSize(), 10u);
   EXPECT_EQ(responseBody(serializeHttp1(response)), "0123456789");
}

TEST(TobasaHttpRangeResponseTest, AdvertisesRangesForFullFileResponse)
{
   TemporaryDirectory tempDir;
   const auto filePath = tempDir.writeFile("payload.bin", "0123456789");
   auto request = makeRequest();
   auto response = makeFileResponse(filePath, true);

   response->prepareFileRangeResponse(*request);

   EXPECT_EQ(response->statusCode(), StatusCode::OK);
   EXPECT_EQ(response->headers().value("Accept-Ranges"), "bytes");
   EXPECT_EQ(response->contentSize(), 10u);
}

TEST(TobasaHttpRangeResponseTest, IgnoresMalformedAndMultipleRanges)
{
   TemporaryDirectory tempDir;
   const auto filePath = tempDir.writeFile("payload.bin", "0123456789");
   const std::vector<std::string> unsupportedRanges = {
      "items=1-2", "bytes=abc", "bytes=2-1", "bytes=1-2,4-5"
   };

   for (const auto& range : unsupportedRanges)
   {
      auto request = makeRequest();
      request->setHeader("Range", range);
      auto response = makeFileResponse(filePath, true);

      response->prepareFileRangeResponse(*request);

      EXPECT_EQ(response->statusCode(), StatusCode::OK) << range;
      EXPECT_EQ(response->contentSize(), 10u) << range;
   }

   auto duplicateRequest = makeRequest();
   duplicateRequest->addHeader("Range", "bytes=0-1");
   duplicateRequest->addHeader("Range", "bytes=2-3");
   auto duplicateResponse = makeFileResponse(filePath, true);
   duplicateResponse->prepareFileRangeResponse(*duplicateRequest);
   EXPECT_EQ(duplicateResponse->statusCode(), StatusCode::OK);
   EXPECT_EQ(duplicateResponse->contentSize(), 10u);
}

TEST(TobasaHttpRangeResponseTest, Returns416ForValidUnsatisfiableRange)
{
   TemporaryDirectory tempDir;
   const auto filePath = tempDir.writeFile("payload.bin", "0123456789");
   auto request = makeRequest();
   request->setHeader("Range", "bytes=10-");
   auto response = makeFileResponse(filePath, true);
   response->useChunkedEncoding(true);

   response->prepareFileRangeResponse(*request);

   EXPECT_EQ(response->statusCode(), StatusCode::RANGE_NOT_SATISFIABLE);
   EXPECT_EQ(response->headers().value("Content-Range"), "bytes */10");
   EXPECT_EQ(response->contentSize(), 0u);
   const auto wireResponse = serializeHttp1(response);
   EXPECT_TRUE(responseBody(wireResponse).empty());
   EXPECT_NE(wireResponse.find("Content-Length: 0"), std::string::npos);
   EXPECT_EQ(wireResponse.find("Transfer-Encoding: chunked"), std::string::npos);
}

TEST(TobasaHttpRangeResponseTest, RequiresMatchingIfRangeValidator)
{
   TemporaryDirectory tempDir;
   const auto filePath = tempDir.writeFile("payload.bin", "0123456789");

   auto matchingRequest = makeRequest();
   matchingRequest->setHeader("Range", "bytes=1-2");
   matchingRequest->setHeader("If-Range", "\"v1\"");
   auto matchingResponse = makeFileResponse(filePath, true);
   matchingResponse->setHeader("ETag", "\"v1\"");
   matchingResponse->prepareFileRangeResponse(*matchingRequest);
   EXPECT_EQ(matchingResponse->statusCode(), StatusCode::PARTIAL_CONTENT);

   auto mismatchingRequest = makeRequest();
   mismatchingRequest->setHeader("Range", "bytes=1-2");
   mismatchingRequest->setHeader("If-Range", "\"old\"");
   auto mismatchingResponse = makeFileResponse(filePath, true);
   mismatchingResponse->setHeader("ETag", "\"v1\"");
   mismatchingResponse->prepareFileRangeResponse(*mismatchingRequest);
   EXPECT_EQ(mismatchingResponse->statusCode(), StatusCode::OK);

   auto missingValidatorRequest = makeRequest();
   missingValidatorRequest->setHeader("Range", "bytes=1-2");
   missingValidatorRequest->setHeader("If-Range", "\"v1\"");
   auto missingValidatorResponse = makeFileResponse(filePath, true);
   missingValidatorResponse->prepareFileRangeResponse(*missingValidatorRequest);
   EXPECT_EQ(missingValidatorResponse->statusCode(), StatusCode::OK);
}

TEST(TobasaHttpRangeResponseTest, HonorsIfRangeLastModifiedDate)
{
   TemporaryDirectory tempDir;
   const auto filePath = tempDir.writeFile("payload.bin", "0123456789");
   const std::string lastModified = "Wed, 21 Oct 2015 07:28:00 GMT";

   auto matchingRequest = makeRequest();
   matchingRequest->setHeader("Range", "bytes=1-2");
   matchingRequest->setHeader("If-Range", lastModified);
   auto matchingResponse = makeFileResponse(filePath, true);
   matchingResponse->setHeader("Last-Modified", lastModified);
   matchingResponse->prepareFileRangeResponse(*matchingRequest);
   EXPECT_EQ(matchingResponse->statusCode(), StatusCode::PARTIAL_CONTENT);

   auto mismatchingRequest = makeRequest();
   mismatchingRequest->setHeader("Range", "bytes=1-2");
   mismatchingRequest->setHeader("If-Range", lastModified);
   auto mismatchingResponse = makeFileResponse(filePath, true);
   mismatchingResponse->setHeader("Last-Modified", "Thu, 22 Oct 2015 07:28:00 GMT");
   mismatchingResponse->prepareFileRangeResponse(*mismatchingRequest);
   EXPECT_EQ(mismatchingResponse->statusCode(), StatusCode::OK);
}

TEST(TobasaHttpRangeResponseTest, IgnoresRangeForHeadAndSerializesNoBody)
{
   TemporaryDirectory tempDir;
   const auto filePath = tempDir.writeFile("payload.bin", "0123456789");
   auto request = makeRequest("GET");
   request->isHeadRequest(true);
   request->setHeader("Range", "bytes=1-2");
   auto response = makeFileResponse(filePath, true);
   response->isHeadRequest(true);

   response->prepareFileRangeResponse(*request);

   EXPECT_EQ(response->statusCode(), StatusCode::OK);
   const auto wireResponse = serializeHttp1(response);
   EXPECT_EQ(responseBody(wireResponse), "");
   EXPECT_NE(wireResponse.find("Content-Length: 10"), std::string::npos);
}

TEST(TobasaHttpRangeResponseTest, Http2SerializerReadsSelectedWindowAcrossBuffers)
{
   TemporaryDirectory tempDir;
   const auto filePath = tempDir.writeFile("payload.bin", "0123456789");
   auto request = makeRequest();
   request->setHeader("Range", "bytes=3-7");
   auto response = makeFileResponse(filePath, true);

   response->prepareFileRangeResponse(*request);
   ResponseSerializer serializer(response);

   EXPECT_EQ(serializeHttp2Body(serializer, 2), "34567");
   EXPECT_EQ(serializer.readLeft(), 0u);
}

TEST(TobasaHttpRangeResponseTest, SerializesChunkedHttp1RangeAndTerminator)
{
   TemporaryDirectory tempDir;
   const auto filePath = tempDir.writeFile("payload.bin", "0123456789");
   auto request = makeRequest();
   request->setHeader("Range", "bytes=2-5");
   auto response = makeFileResponse(filePath, true);
   response->useChunkedEncoding(true);

   response->prepareFileRangeResponse(*request);
   const auto wireResponse = serializeHttp1(response, 3);
   const auto body = responseBody(wireResponse);

   EXPECT_EQ(response->statusCode(), StatusCode::PARTIAL_CONTENT);
   EXPECT_NE(wireResponse.find("Transfer-Encoding: chunked"), std::string::npos);
   EXPECT_NE(body.find("3\r\n234\r\n1\r\n5\r\n0\r\n\r\n"), std::string::npos);
}