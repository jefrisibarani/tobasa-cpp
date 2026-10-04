#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

#include "tobasahttp/http_parser.h"

TEST(TobasaHttpParserTest, ParsesCompleteGetRequest)
{
   const std::string request =
      "GET /health?full=1 HTTP/1.1\r\n"
      "Host: example.test\r\n"
      "Connection: close\r\n"
      "\r\n";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto info = parser.parse(buffer.size());

   ASSERT_TRUE(info.success());
   EXPECT_TRUE(parser.done());
   EXPECT_TRUE(parser.headersDone());
   EXPECT_TRUE(parser.contentDone());
   EXPECT_EQ(parser.method(), "GET");
   EXPECT_EQ(parser.requestTarget(), "/health?full=1");
   EXPECT_EQ(parser.requestLine(), "GET /health?full=1 HTTP/1.1");
   EXPECT_EQ(parser.majorVersion(), 1U);
   EXPECT_EQ(parser.minorVersion(), 1U);

   const auto host = parser.findHeader("host");
   ASSERT_TRUE(host.valid());
   EXPECT_EQ(host.value(), "example.test");
}

TEST(TobasaHttpParserTest, ParsesRequestTargetForms)
{
   const std::vector<std::pair<std::string, std::string>> requests = {
      {"GET", "/resource?q=1"},
      {"GET", "http://example.test/resource"},
      {"OPTIONS", "*"},
      {"CONNECT", "example.test:443"}
   };

   for (const auto& [method, target] : requests)
   {
      const std::string request =
         method + " " + target + " HTTP/1.1\r\nHost: example.test\r\n\r\n";
      std::vector<char> buffer(request.begin(), request.end());
      tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

      auto info = parser.parse(buffer.size());

      ASSERT_TRUE(info.success()) << request;
      EXPECT_EQ(parser.method(), method);
      EXPECT_EQ(parser.requestTarget(), target);
      EXPECT_TRUE(parser.done());
   }
}

TEST(TobasaHttpParserTest, ParsesHttpResponseAndContentLengthBody)
{
   const std::string response =
      "HTTP/1.1 200 OK\r\n"
      "Content-Length: 2\r\n"
      "\r\n"
      "OK";
   std::vector<char> buffer(response.begin(), response.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::RESPONSE, buffer);

   auto info = parser.parse(buffer.size());

   ASSERT_TRUE(info.success());
   EXPECT_TRUE(parser.done());
   EXPECT_EQ(parser.statusCode(), 200U);
   EXPECT_EQ(parser.statusMessage(), "OK");
   EXPECT_EQ(parser.content(), "OK");
}

TEST(TobasaHttpParserTest, RejectsMalformedRequestLines)
{
   const std::vector<std::string> requests = {
      "GET / ABCD/1.1\r\nHost: example.test\r\n\r\n",
      "GET / HTTP/1.2\r\nHost: example.test\r\n\r\n",
      "GET  HTTP/1.1\r\nHost: example.test\r\n\r\n",
      "GET /\r\nHost: example.test\r\n\r\n"
   };

   for (const auto& request : requests)
   {
      std::vector<char> buffer(request.begin(), request.end());
      tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

      auto info = parser.parse(buffer.size());

      EXPECT_FALSE(info.success()) << request;
   }
}

TEST(TobasaHttpParserTest, RejectsMalformedResponseVersion)
{
   const std::string response = "ABCD/1.1 200 OK\r\n\r\n";
   std::vector<char> buffer(response.begin(), response.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::RESPONSE, buffer);

   auto info = parser.parse(buffer.size());

   EXPECT_FALSE(info.success());
}

TEST(TobasaHttpParserTest, ParsesRequestWhenSplitAtEveryByte)
{
   const std::string request =
      "GET /fragmented HTTP/1.1\r\n"
      "Host: example.test\r\n"
      "\r\n";
   std::vector<char> buffer;
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   for (size_t index = 0; index < request.size(); ++index)
   {
      buffer.assign(1, request[index]);
      auto info = parser.parse(buffer.size());
      ASSERT_TRUE(info.success()) << "byte index " << index;
   }

   EXPECT_TRUE(parser.done());
   EXPECT_EQ(parser.requestTarget(), "/fragmented");
   EXPECT_EQ(parser.findHeader("Host").value(), "example.test");
}

TEST(TobasaHttpParserTest, PreservesHeaderWhitespaceAndFindsEmptyValue)
{
   const std::string request =
      "GET / HTTP/1.1\r\n"
      "Host: example.test\r\n"
      "X-Note: hello\tworld\r\n"
      "X-Empty:\r\n"
      "\r\n";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto info = parser.parse(buffer.size());

   ASSERT_TRUE(info.success());
   EXPECT_EQ(parser.findHeader("host").value(), "example.test");
   EXPECT_EQ(parser.findHeader("X-Note").value(), "hello\tworld");
   const auto empty = parser.findHeader("X-Empty");
   ASSERT_TRUE(empty.valid());
   EXPECT_TRUE(empty.value().empty());
}

TEST(TobasaHttpParserTest, RejectsWhitespaceBeforeHeaderColon)
{
   const std::string request =
      "GET / HTTP/1.1\r\n"
      "Host : example.test\r\n"
      "\r\n";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto info = parser.parse(buffer.size());

   EXPECT_FALSE(info.success());
}

TEST(TobasaHttpParserTest, RejectsHeadersOverConfiguredLimit)
{
   const std::string request =
      "GET / HTTP/1.1\r\n"
      "Host: example.test\r\n"
      "\r\n";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer, 5);

   auto info = parser.parse(buffer.size());

   EXPECT_FALSE(info.success());
   EXPECT_EQ(info.httpStatus().code(), tbs::http::StatusCode::REQUEST_HEADER_FIELDS_TOO_LARGE);
}

TEST(TobasaHttpParserTest, ReadsContentLengthBodyAcrossFragments)
{
   const std::string header =
      "POST /submit HTTP/1.1\r\n"
      "Host: example.test\r\n"
      "Content-Length: 4\r\n"
      "\r\n";
   std::vector<char> buffer(header.begin(), header.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto headerInfo = parser.parse(buffer.size());
   ASSERT_TRUE(headerInfo.success());
   EXPECT_FALSE(parser.contentDone());

   buffer.assign({'a', 'b'});
   auto firstBodyInfo = parser.parse(buffer.size());
   ASSERT_TRUE(firstBodyInfo.success());
   EXPECT_FALSE(parser.contentDone());

   buffer.assign({'c', 'd'});
   auto secondBodyInfo = parser.parse(buffer.size());
   ASSERT_TRUE(secondBodyInfo.success());
   EXPECT_TRUE(parser.done());
   EXPECT_EQ(parser.content(), "abcd");
}

TEST(TobasaHttpParserTest, ConsumesOnlyContentLengthBytes)
{
   const std::string request =
      "POST /submit HTTP/1.1\r\n"
      "Host: example.test\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "abcNEXT";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto info = parser.parse(buffer.size());

   ASSERT_TRUE(info.success()) << info.message();
   EXPECT_TRUE(parser.done());
   EXPECT_EQ(parser.content(), "abc");
}

TEST(TobasaHttpParserTest, HonorsContentLengthForGetRequests)
{
   const std::string request =
      "GET /resource HTTP/1.1\r\n"
      "Host: example.test\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "abc";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto info = parser.parse(buffer.size());

   ASSERT_TRUE(info.success());
   EXPECT_TRUE(parser.done());
   EXPECT_EQ(parser.content(), "abc");
}

TEST(TobasaHttpParserTest, RejectsMalformedContentLength)
{
   const std::vector<std::string> values = {"3x", "+3", "-1", "999999999999999999999999999999"};

   for (const auto& value : values)
   {
      const std::string request =
         "POST /submit HTTP/1.1\r\n"
         "Host: example.test\r\n"
         "Content-Length: " + value + "\r\n\r\n";
      std::vector<char> buffer(request.begin(), request.end());
      tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

      EXPECT_NO_THROW({
         auto info = parser.parse(buffer.size());
         EXPECT_FALSE(info.success()) << value;
      });
   }
}

TEST(TobasaHttpParserTest, RejectsConflictingContentLengths)
{
   const std::string request =
      "POST /submit HTTP/1.1\r\n"
      "Host: example.test\r\n"
      "Content-Length: 3\r\n"
      "Content-Length: 4\r\n"
      "\r\n"
      "abcd";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto info = parser.parse(buffer.size());

   EXPECT_FALSE(info.success());
}

TEST(TobasaHttpParserTest, RejectsContentLengthWithChunkedTransferEncoding)
{
   const std::string request =
      "POST /submit HTTP/1.1\r\n"
      "Host: example.test\r\n"
      "Content-Length: 3\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"
      "3\r\nabc\r\n0\r\n\r\n";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto info = parser.parse(buffer.size());

   EXPECT_FALSE(info.success());
}

TEST(TobasaHttpParserTest, ParsesMultipleChunksAndTrailer)
{
   const std::string request =
      "POST /upload HTTP/1.1\r\n"
      "Host: example.test\r\n"
      "Transfer-Encoding: chunked\r\n"
      "Trailer: X-Checksum\r\n"
      "\r\n"
      "4\r\nWiki\r\n"
      "5\r\npedia\r\n"
      "0\r\n"
      "X-Checksum: yes\r\n"
      "\r\n";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto info = parser.parse(buffer.size());

   ASSERT_TRUE(info.success());
   EXPECT_TRUE(parser.done());
   EXPECT_EQ(parser.content(), "Wikipedia");
}

TEST(TobasaHttpParserTest, AcceptsChunkExtensionsAndCaseInsensitiveCoding)
{
   const std::string request =
      "POST /upload HTTP/1.1\r\n"
      "Host: example.test\r\n"
      "Transfer-Encoding: Chunked\r\n"
      "\r\n"
      "3;name=value\r\nabc\r\n"
      "0\r\n\r\n";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto info = parser.parse(buffer.size());

   ASSERT_TRUE(info.success());
   EXPECT_TRUE(parser.done());
   EXPECT_EQ(parser.content(), "abc");
}

TEST(TobasaHttpParserTest, RejectsMalformedChunkSize)
{
   const std::string request =
      "POST /upload HTTP/1.1\r\n"
      "Host: example.test\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"
      "Z\r\n";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto info = parser.parse(buffer.size());

   EXPECT_FALSE(info.success());
}

TEST(TobasaHttpParserTest, CompletesBodylessResponseStatuses)
{
   for (const auto statusLine : {
           "HTTP/1.1 100 Continue",
           "HTTP/1.1 204 No Content",
           "HTTP/1.1 304 Not Modified"})
   {
      const std::string response = std::string(statusLine) + "\r\n\r\n";
      std::vector<char> buffer(response.begin(), response.end());
      tbs::http::parser::Parser parser(tbs::http::parser::Type::RESPONSE, buffer);

      auto info = parser.parse(buffer.size());

      ASSERT_TRUE(info.success()) << statusLine;
      EXPECT_TRUE(parser.done()) << statusLine;
      EXPECT_TRUE(parser.content().empty());
   }
}

TEST(TobasaHttpParserTest, ParsesTrailerWithoutTrailerDeclaration)
{
   const std::string request =
      "POST /upload HTTP/1.1\r\n"
      "Host: example.test\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"
      "1\r\nx\r\n"
      "0\r\n"
      "X-Checksum: yes\r\n"
      "\r\n";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto info = parser.parse(buffer.size());

   EXPECT_TRUE(info.success());
   EXPECT_TRUE(parser.done());
   EXPECT_EQ(parser.content(), "x");
}

TEST(TobasaHttpParserTest, ParsesChunkedBodyAcrossFragments)
{
   const std::string header =
      "POST /upload HTTP/1.1\r\n"
      "Host: example.test\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n";
   std::vector<char> buffer(header.begin(), header.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto headerInfo = parser.parse(buffer.size());
   ASSERT_TRUE(headerInfo.success());

   const std::string body = "3\r\nabc\r\n0\r\n\r\n";
   for (size_t index = 0; index < body.size(); ++index)
   {
      buffer.assign(1, body[index]);
      auto info = parser.parse(buffer.size());
      ASSERT_TRUE(info.success()) << "body byte index " << index;
   }

   EXPECT_TRUE(parser.done());
   EXPECT_EQ(parser.content(), "abc");
}

TEST(TobasaHttpParserTest, WaitsForFragmentedChunkTrailerTerminator)
{
   const std::string header =
      "POST /upload HTTP/1.1\r\n"
      "Host: example.test\r\n"
      "Transfer-Encoding: chunked\r\n"
      "Trailer: X-Checksum\r\n"
      "\r\n";
   std::vector<char> buffer(header.begin(), header.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto headerInfo = parser.parse(buffer.size());
   ASSERT_TRUE(headerInfo.success());

   const std::string body = "1\r\nx\r\n0\r\nX-Checksum: yes\r\n\r\n";
   for (size_t index = 0; index < body.size(); ++index)
   {
      buffer.assign(1, body[index]);
      auto info = parser.parse(buffer.size());
      ASSERT_TRUE(info.success()) << "body byte index " << index << ": " << info.message();
      if (index + 1 < body.size())
         EXPECT_FALSE(parser.done()) << "body byte index " << index;
   }

   EXPECT_TRUE(parser.done());
   EXPECT_EQ(parser.content(), "x");
}

TEST(TobasaHttpParserTest, AppliesRequestMethodValidationCallback)
{
   const std::string request = "BREW / HTTP/1.1\r\nHost: example.test\r\n\r\n";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);
   parser.onValidateRequestMethod = [](const std::string&, tbs::http::HttpStatus& status)
   {
      status = tbs::http::HttpStatus(tbs::http::StatusCode::METHOD_NOT_ALLOWED);
      return false;
   };

   auto info = parser.parse(buffer.size());

   EXPECT_FALSE(info.success());
   EXPECT_EQ(info.httpStatus().code(), tbs::http::StatusCode::METHOD_NOT_ALLOWED);
}

TEST(TobasaHttpParserTest, AppliesHeaderValidationCallback)
{
   const std::string request = "GET / HTTP/1.1\r\nHost: example.test\r\n\r\n";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);
   parser.onValidateHeaders = [](tbs::http::parser::Parser&, tbs::http::HttpStatus& status)
   {
      status = tbs::http::HttpStatus(tbs::http::StatusCode::BAD_REQUEST);
      return false;
   };

   auto info = parser.parse(buffer.size());

   EXPECT_FALSE(info.success());
   EXPECT_EQ(info.httpStatus().code(), tbs::http::StatusCode::BAD_REQUEST);
}

TEST(TobasaHttpParserTest, ReportsExpectContinueBeforeReadingBody)
{
   const std::string request =
      "POST /submit HTTP/1.1\r\n"
      "Host: example.test\r\n"
      "Content-Length: 4\r\n"
      "Expect: 100-continue\r\n"
      "\r\n";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto info = parser.parse(buffer.size());

   EXPECT_TRUE(info.success());
   EXPECT_EQ(info.httpStatus().code(), tbs::http::StatusCode::CONTINUE);
   EXPECT_FALSE(parser.contentDone());
}

TEST(TobasaHttpParserTest, ResetsForNextMessage)
{
   std::string request = "GET /first HTTP/1.1\r\nHost: example.test\r\n\r\n";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto firstInfo = parser.parse(buffer.size());
   ASSERT_TRUE(firstInfo.success());
   ASSERT_TRUE(parser.done());

   parser.prepareForNextMessage();
   request = "GET /second HTTP/1.0\r\nHost: example.test\r\n\r\n";
   buffer.assign(request.begin(), request.end());

   auto secondInfo = parser.parse(buffer.size());

   ASSERT_TRUE(secondInfo.success());
   EXPECT_TRUE(parser.done());
   EXPECT_EQ(parser.requestTarget(), "/second");
   EXPECT_EQ(parser.minorVersion(), 0U);
}

TEST(TobasaHttpParserTest, RejectsControlCharactersInRequestLine)
{
   std::string request = "GET /";
   request.push_back('\x15');
   request += " HTTP/1.1\r\nHost: example.test\r\n\r\n";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto info = parser.parse(buffer.size());

   EXPECT_FALSE(info.success());
}

TEST(TobasaHttpParserTest, RejectsInvalidHeaderNameCharacters)
{
   for (const auto& headerName : {"Bad Name", "Bad@Name"})
   {
      const std::string request =
         "GET / HTTP/1.1\r\n"
         "Host: example.test\r\n" + std::string(headerName) + ": value\r\n\r\n";
      std::vector<char> buffer(request.begin(), request.end());
      tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

      auto info = parser.parse(buffer.size());

      EXPECT_FALSE(info.success()) << headerName;
   }
}

TEST(TobasaHttpParserTest, RejectsControlCharactersInHeaderValues)
{
   std::string request = "GET / HTTP/1.1\r\nHost: example.test\r\nX-Value: a";
   request.push_back('\x01');
   request += "b\r\n\r\n";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto info = parser.parse(buffer.size());

   EXPECT_FALSE(info.success());
}

TEST(TobasaHttpParserTest, RejectsMalformedCrLfSequences)
{
   const std::vector<std::string> requests = {
      "GET / HTTP/1.1\rX\nHost: example.test\r\n\r\n",
      "GET / HTTP/1.1\r\nHost: example.test\rX\n\r\n"
   };

   for (const auto& request : requests)
   {
      std::vector<char> buffer(request.begin(), request.end());
      tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

      auto info = parser.parse(buffer.size());

      EXPECT_FALSE(info.success()) << request;
   }
}

TEST(TobasaHttpParserTest, AcceptsIdenticalDuplicateContentLengths)
{
   const std::string request =
      "POST /submit HTTP/1.1\r\n"
      "Host: example.test\r\n"
      "Content-Length: 3\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "abc";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto info = parser.parse(buffer.size());

   ASSERT_TRUE(info.success());
   EXPECT_TRUE(parser.done());
   EXPECT_EQ(parser.content(), "abc");
}

TEST(TobasaHttpParserTest, ReportsHeaderBeforeBodyCompletion)
{
   const std::string request =
      "POST /submit HTTP/1.1\r\n"
      "Host: example.test\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "abc";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);
   std::vector<std::string> events;
   parser.onEventDone = [&events](const std::string& eventType)
   {
      events.push_back(eventType);
   };

   auto info = parser.parse(buffer.size());

   ASSERT_TRUE(info.success()) << info.message();
   EXPECT_EQ(events, (std::vector<std::string>{"headers", "body"}));
}

TEST(TobasaHttpParserTest, ParsesPipelinedBodylessRequestsSeparately)
{
   const std::string first = "GET /first HTTP/1.1\r\nHost: example.test\r\n\r\n";
   const std::string second = "GET /second HTTP/1.1\r\nHost: example.test\r\n\r\n";
   const std::string pipelined = first + second;
   std::vector<char> buffer(pipelined.begin(), pipelined.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto firstInfo = parser.parse(buffer.size());

   ASSERT_TRUE(firstInfo.success()) << firstInfo.message();
   ASSERT_TRUE(parser.done());
   const size_t nextMessageOffset = firstInfo.lastIndex() + 1;
   ASSERT_EQ(nextMessageOffset, first.size());

   parser.prepareForNextMessage();
   buffer.assign(pipelined.begin() + nextMessageOffset, pipelined.end());
   auto secondInfo = parser.parse(buffer.size());

   ASSERT_TRUE(secondInfo.success());
   EXPECT_TRUE(parser.done());
   EXPECT_EQ(parser.requestTarget(), "/second");
}

TEST(TobasaHttpParserTest, ParsesNextRequestAfterContentLengthBody)
{
   const std::string first =
      "POST /submit HTTP/1.1\r\n"
      "Host: example.test\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "abc";
   const std::string second = "GET /next HTTP/1.1\r\nHost: example.test\r\n\r\n";
   const std::string pipelined = first + second;
   std::vector<char> buffer(pipelined.begin(), pipelined.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto firstInfo = parser.parse(buffer.size());

   ASSERT_TRUE(firstInfo.success()) << firstInfo.message();
   ASSERT_TRUE(parser.done());
   EXPECT_EQ(parser.content(), "abc");
   const size_t nextMessageOffset = firstInfo.lastIndex() + 1;
   ASSERT_EQ(nextMessageOffset, first.size());

   parser.prepareForNextMessage();
   buffer.assign(pipelined.begin() + nextMessageOffset, pipelined.end());
   auto secondInfo = parser.parse(buffer.size());

   ASSERT_TRUE(secondInfo.success()) << secondInfo.message();
   EXPECT_TRUE(parser.done());
   EXPECT_EQ(parser.requestTarget(), "/next");
}

TEST(TobasaHttpParserTest, EnforcesRequestTargetLengthLimit)
{
   const std::string target(4097, 'a');
   const std::string request =
      "GET /" + target + " HTTP/1.1\r\nHost: example.test\r\n\r\n";
   std::vector<char> buffer(request.begin(), request.end());
   tbs::http::parser::Parser parser(tbs::http::parser::Type::REQUEST, buffer);

   auto info = parser.parse(buffer.size());

   EXPECT_FALSE(info.success());
   EXPECT_EQ(info.httpStatus().code(), tbs::http::StatusCode::URI_TOO_LONG) << info.message();
}