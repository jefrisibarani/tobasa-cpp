#include <gtest/gtest.h>

#include <asio.hpp>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "tobasa_test_support/environment.h"
#include "tobasahttp/client/http_client.h"

namespace {

class TestHttpSession : public std::enable_shared_from_this<TestHttpSession>
{
   asio::ip::tcp::socket _socket;
   asio::streambuf _request;

public:
   explicit TestHttpSession(asio::ip::tcp::socket socket)
      : _socket(std::move(socket))
   {}

   void readRequest()
   {
      auto self = shared_from_this();
      asio::async_read_until(_socket, _request, "\r\n\r\n",
         [self](const asio::error_code& error, std::size_t)
         {
            if (error)
               return;

            std::istream requestStream(&self->_request);
            std::string requestLine;
            std::getline(requestStream, requestLine);
            std::istringstream requestLineStream(requestLine);
            std::string method;
            std::string target;
            requestLineStream >> method >> target;

            std::string headerLine;
            std::size_t contentLength = 0;
            while (std::getline(requestStream, headerLine) && headerLine != "\r")
            {
               constexpr std::string_view contentLengthHeader = "Content-Length: ";
               if (headerLine.rfind(contentLengthHeader.data(), 0) == 0)
                  contentLength = static_cast<std::size_t>(std::stoull(
                     headerLine.substr(contentLengthHeader.size())));
            }

            if (target == "/disconnect")
            {
               asio::error_code ignored;
               self->_socket.close(ignored);
               return;
            }

            auto sendResponse = [self, method, target, contentLength]()
            {
               std::string requestBody(contentLength, '\0');
               if (contentLength > 0)
               {
                  std::istream bodyStream(&self->_request);
                  bodyStream.read(requestBody.data(), static_cast<std::streamsize>(contentLength));
                  if (static_cast<std::size_t>(bodyStream.gcount()) != contentLength)
                     return;
               }

               if (target == "/chunked")
               {
                  auto response = std::make_shared<std::string>(
                     "HTTP/1.1 200 OK\r\nX-Test-Request-Method: " + method +
                     "\r\nTransfer-Encoding: chunked\r\n\r\n"
                     "4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n");
                  asio::async_write(self->_socket, asio::buffer(*response),
                     [self, response](const asio::error_code& writeError, std::size_t)
                     {
                        if (!writeError)
                           self->readRequest();
                     });
                  return;
               }

               const auto responseContent = method == "HEAD"
                  ? std::string()
                  : target == "/upload" ? requestBody : target;
               auto response = std::make_shared<std::string>(
                  "HTTP/1.1 200 OK\r\nX-Test-Request-Method: " + method +
                  "\r\nContent-Length: " +
                  std::to_string(responseContent.size()) + "\r\n\r\n" + responseContent);
               asio::async_write(self->_socket, asio::buffer(*response),
                  [self, response](const asio::error_code& writeError, std::size_t)
                  {
                     if (!writeError)
                        self->readRequest();
                  });
            };

            const auto bufferedBodySize = self->_request.size();
            if (bufferedBodySize < contentLength)
            {
               asio::async_read(self->_socket, self->_request,
                  asio::transfer_exactly(contentLength - bufferedBodySize),
                  [sendResponse = std::move(sendResponse)]
                  (const asio::error_code& bodyError, std::size_t) mutable
                  {
                     if (!bodyError)
                        sendResponse();
                  });
            }
            else
               sendResponse();
         });
   }
};

class TestHttpServer
{
   asio::io_context _io;
   asio::ip::tcp::acceptor _acceptor;
   std::atomic<std::size_t> _connectionCount{0};
   std::thread _worker;

   void acceptNext()
   {
      _acceptor.async_accept(
         [this](const asio::error_code& error, asio::ip::tcp::socket socket)
         {
            if (!error)
            {
               ++_connectionCount;
               std::make_shared<TestHttpSession>(std::move(socket))->readRequest();
            }

            if (_acceptor.is_open())
               acceptNext();
         });
   }

public:
   TestHttpServer()
      : _acceptor(_io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 0))
   {
      acceptNext();
      _worker = std::thread([this] { _io.run(); });
   }

   ~TestHttpServer()
   {
      _io.stop();
      _acceptor.close();
      if (_worker.joinable())
         _worker.join();
   }

   std::uint16_t port() const
   {
      return _acceptor.local_endpoint().port();
   }

   std::size_t connectionCount() const
   {
      return _connectionCount.load();
   }
};

class TestDisconnectServer
{
   asio::io_context _io;
   asio::ip::tcp::acceptor _acceptor;
   std::thread _worker;

public:
   TestDisconnectServer()
      : _acceptor(_io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 0))
   {
      _acceptor.async_accept(
         [](const asio::error_code& error, asio::ip::tcp::socket socket)
         {
            if (!error)
            {
               asio::error_code ignored;
               socket.close(ignored);
            }
         });
      _worker = std::thread([this] { _io.run(); });
   }

   ~TestDisconnectServer()
   {
      _io.stop();
      _acceptor.close();
      if (_worker.joinable())
         _worker.join();
   }

   std::uint16_t port() const
   {
      return _acceptor.local_endpoint().port();
   }
};

std::uint16_t closedPort()
{
   asio::io_context io;
   asio::ip::tcp::acceptor acceptor(
      io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 0));
   const auto port = acceptor.local_endpoint().port();
   acceptor.close();
   return port;
}

} // namespace

TEST(HttpClientPoolTest, RepeatedSyncRequestsUseTheirOwnResponse)
{
   TestHttpServer server;
   tbs::log::StdoutLogger logger;
   tbs::http::SettingsClient settings("127.0.0.1", server.port());
   settings.connectionPoolSize(1);
   tbs::http::PlainClientDefault client(std::move(settings), logger);

   auto first = client.get("/first");
   auto second = client.get("/second");

   ASSERT_TRUE(first.responseReceived());
   ASSERT_TRUE(second.responseReceived());
   EXPECT_EQ(first.response->content(), "/first");
   EXPECT_EQ(second.response->content(), "/second");
   EXPECT_EQ(server.connectionCount(), 1u);
   client.shutdown();
}

TEST(HttpClientPoolTest, StreamedResponseHandlerDoesNotLeakToNextRequest)
{
   TestHttpServer server;
   tbs::log::StdoutLogger logger;
   tbs::http::SettingsClient settings("127.0.0.1", server.port());
   settings.connectionPoolSize(1);
   tbs::http::PlainClientDefault client(std::move(settings), logger);
   std::string streamedBody;
   std::size_t streamedChunkCount = 0;

   tbs::http::ClientRequest request("GET", "/streamed");
   request.responseBodyHandler([&](const uint8_t *data, size_t size)
   {
      streamedBody.append(reinterpret_cast<const char *>(data), size);
      ++streamedChunkCount;
   });
   auto streamedResult = client.execute(std::move(request));
   auto bufferedResult = client.get("/buffered");

   ASSERT_TRUE(streamedResult.responseReceived());
   ASSERT_TRUE(bufferedResult.responseReceived());
   EXPECT_EQ(streamedBody, "/streamed");
   EXPECT_GT(streamedChunkCount, 0u);
   EXPECT_TRUE(streamedResult.response->content().empty());
   EXPECT_EQ(bufferedResult.response->content(), "/buffered");
   EXPECT_EQ(streamedChunkCount, 1u);
   EXPECT_EQ(server.connectionCount(), 1u);
   client.shutdown();
}

TEST(HttpClientMethodsTest, StreamsChunkedResponseBody)
{
   TestHttpServer server;
   tbs::log::StdoutLogger logger;
   tbs::http::SettingsClient settings("127.0.0.1", server.port());
   tbs::http::PlainClientDefault client(std::move(settings), logger);
   std::string streamedBody;

   tbs::http::ClientRequest request("GET", "/chunked");
   request.responseBodyHandler([&](const uint8_t *data, size_t size)
   {
      streamedBody.append(reinterpret_cast<const char *>(data), size);
   });
   auto result = client.execute(std::move(request));

   ASSERT_TRUE(result.responseReceived());
   EXPECT_EQ(result.response->statusCode(), 200U);
   EXPECT_EQ(streamedBody, "Wikipedia");
   EXPECT_TRUE(result.response->content().empty());
   client.shutdown();
}

TEST(HttpClientMethodsTest, ResponseHeadersArriveBeforeBodyChunks)
{
   TestHttpServer server;
   tbs::log::StdoutLogger logger;
   tbs::http::SettingsClient settings("127.0.0.1", server.port());
   tbs::http::PlainClientDefault client(std::move(settings), logger);
   bool headersReceived = false;
   std::string contentLength;
   std::string streamedBody;
   tbs::http::ClientResponsePtr headersResponse;

   tbs::http::ClientRequest request("GET", "/headers-first");
   request.responseHeadersHandler([&](const tbs::http::ClientResponsePtr& response)
   {
      headersReceived = true;
      headersResponse = response;
      contentLength = response->headers().value("Content-Length");
      EXPECT_EQ(response->statusCode(), 200U);
   });
   request.responseBodyHandler([&](const uint8_t *data, size_t size)
   {
      EXPECT_TRUE(headersReceived);
      streamedBody.append(reinterpret_cast<const char *>(data), size);
   });

   auto result = client.execute(std::move(request));

   ASSERT_TRUE(result.responseReceived());
   ASSERT_TRUE(headersReceived);
   EXPECT_EQ(contentLength, std::to_string(std::string("/headers-first").size()));
   EXPECT_EQ(streamedBody, "/headers-first");
   EXPECT_EQ(result.response, headersResponse);
   EXPECT_TRUE(result.response->content().empty());
   client.shutdown();
}

TEST(HttpClientMethodsTest, AsyncStreamingUploadAndDownloadCompletesOnce)
{
   TestHttpServer server;
   tbs::log::StdoutLogger logger;
   tbs::http::SettingsClient settings("127.0.0.1", server.port());
   tbs::http::PlainClientDefault client(std::move(settings), logger);
   const std::vector<uint8_t> requestBody{'a', 0, 'b', 'c'};
   std::string streamedBody;
   tbs::http::ClientResult result;
   std::size_t completionCount = 0;

   tbs::http::ClientRequest request("POST", "/upload");
   request.bodySource(tbs::http::RequestDataSource::fromBytes(requestBody));
   request.responseBodyHandler([&](const uint8_t *data, size_t size)
   {
      streamedBody.append(reinterpret_cast<const char *>(data), size);
   });
   client.execute(std::move(request), [&](const tbs::http::ClientResult& requestResult)
   {
      result = requestResult;
      ++completionCount;
   });

   client.shutdown();
   client.run(2);

   ASSERT_TRUE(result.responseReceived()) << result.error.message;
   EXPECT_EQ(completionCount, 1u);
   EXPECT_EQ(streamedBody, std::string("a\0bc", 4));
   EXPECT_TRUE(result.response->content().empty());
}

TEST(HttpClientMethodsTest, StreamsOwnedByteAndFileRequestBodies)
{
   TestHttpServer server;
   tbs::log::StdoutLogger logger;
   tbs::http::SettingsClient settings("127.0.0.1", server.port());
   settings.connectionPoolSize(1);
   tbs::http::PlainClientDefault client(std::move(settings), logger);

   std::vector<uint8_t> byteBody(150000);
   for (std::size_t index = 0; index < byteBody.size(); ++index)
      byteBody[index] = static_cast<uint8_t>(index % 251);
   const std::string expectedByteBody(
      reinterpret_cast<const char *>(byteBody.data()), byteBody.size());

   tbs::http::ClientRequest byteRequest("POST", "/upload");
   byteRequest.bodySource(tbs::http::RequestDataSource::fromBytes(std::move(byteBody)));
   auto byteResult = client.execute(std::move(byteRequest));

   ASSERT_TRUE(byteResult.responseReceived()) << byteResult.error.message;
   EXPECT_EQ(byteResult.response->content(), expectedByteBody);

   const auto temporaryPath = std::filesystem::temp_directory_path() /
      ("tobasa-http-upload-" + std::to_string(
         std::chrono::steady_clock::now().time_since_epoch().count()) + ".bin");
   const std::string expectedFileBody("file\0body", 9);
   {
      std::ofstream file(temporaryPath, std::ios::binary);
      ASSERT_TRUE(file.is_open());
      file.write(expectedFileBody.data(), static_cast<std::streamsize>(expectedFileBody.size()));
      ASSERT_TRUE(file.good());
   }

   tbs::http::ClientRequest fileRequest("POST", "/upload");
   fileRequest.bodySource(tbs::http::RequestDataSource::fromFile(temporaryPath.string()));
   auto fileResult = client.execute(std::move(fileRequest));
   std::error_code ignored;
   std::filesystem::remove(temporaryPath, ignored);

   ASSERT_TRUE(fileResult.responseReceived()) << fileResult.error.message;
   EXPECT_EQ(fileResult.response->content(), expectedFileBody);
   EXPECT_EQ(server.connectionCount(), 1u);
   client.shutdown();
}

TEST(HttpClientMethodsTest, StreamsEmptyRequestBody)
{
   TestHttpServer server;
   tbs::log::StdoutLogger logger;
   tbs::http::SettingsClient settings("127.0.0.1", server.port());
   tbs::http::PlainClientDefault client(std::move(settings), logger);

   tbs::http::ClientRequest request("POST", "/upload");
   request.bodySource(tbs::http::RequestDataSource::fromBytes({}));
   auto result = client.execute(std::move(request));

   ASSERT_TRUE(result.responseReceived());
   EXPECT_TRUE(result.response->content().empty());
   client.shutdown();
}

TEST(HttpClientMethodsTest, RejectsTransferEncodingForKnownSizeSource)
{
   TestHttpServer server;
   tbs::log::StdoutLogger logger;
   tbs::http::SettingsClient settings("127.0.0.1", server.port());
   tbs::http::PlainClientDefault client(std::move(settings), logger);

   tbs::http::ClientRequest request("POST", "/upload");
   request.addHeader("Transfer-Encoding", "chunked");
   request.bodySource(tbs::http::RequestDataSource::fromBytes({1, 2, 3}));
   auto result = client.execute(std::move(request));

   EXPECT_FALSE(result.responseReceived());
   EXPECT_NE(result.error.message.find("Content-Length framing"), std::string::npos);
   client.shutdown();
}

TEST(HttpClientMethodsTest, SyncGetPreservesQueryInRequestTarget)
{
   TestHttpServer server;
   tbs::log::StdoutLogger logger;
   tbs::http::SettingsClient settings("127.0.0.1", server.port());
   tbs::http::PlainClientDefault client(std::move(settings), logger);

   const std::string target = "/search?term=client&offset=2";
   auto result = client.get(target);

   ASSERT_TRUE(result.responseReceived());
   EXPECT_EQ(result.response->content(), target);
   client.shutdown();
}

TEST(HttpClientMethodsTest, SyncHeadAndOptionsSendTheirMethods)
{
   TestHttpServer server;
   tbs::log::StdoutLogger logger;
   tbs::http::SettingsClient settings("127.0.0.1", server.port());
   tbs::http::PlainClientDefault client(std::move(settings), logger);

   auto headResponse = client.head("/head");
   auto optionsResponse = client.options("/options");

   ASSERT_TRUE(headResponse.responseReceived());
   ASSERT_TRUE(optionsResponse.responseReceived());
   EXPECT_EQ(headResponse.response->headers().findHeader("X-Test-Request-Method").value, "HEAD");
   EXPECT_TRUE(headResponse.response->content().empty());
   EXPECT_EQ(optionsResponse.response->headers().findHeader("X-Test-Request-Method").value, "OPTIONS");
   EXPECT_EQ(optionsResponse.response->content(), "/options");
   client.shutdown();
}

TEST(HttpClientMethodsTest, AsyncHeadAndOptionsSendTheirMethods)
{
   TestHttpServer server;
   tbs::log::StdoutLogger logger;
   tbs::http::SettingsClient settings("127.0.0.1", server.port());
   tbs::http::PlainClientDefault client(std::move(settings), logger);

   std::atomic<std::size_t> completed{0};
   std::atomic<std::size_t> mismatchedResponses{0};
   client.head("/head", [&](const tbs::http::ClientResult& result)
   {
      if (!result.responseReceived() ||
          result.response->headers().findHeader("X-Test-Request-Method").value != "HEAD" ||
          !result.response->content().empty())
         ++mismatchedResponses;

      ++completed;
   });
   client.options("/options", [&](const tbs::http::ClientResult& result)
   {
      if (!result.responseReceived() ||
          result.response->headers().findHeader("X-Test-Request-Method").value != "OPTIONS" ||
          result.response->content() != "/options")
         ++mismatchedResponses;

      ++completed;
   });

   client.shutdown();
   client.run();

   EXPECT_EQ(completed.load(), 2u);
   EXPECT_EQ(mismatchedResponses.load(), 0u);
}

TEST(HttpClientResultTest, SyncSuccessRetainsResponseForCustomMethod)
{
   TestHttpServer server;
   tbs::log::StdoutLogger logger;
   tbs::http::SettingsClient settings("127.0.0.1", server.port());
   tbs::http::PlainClientDefault client(std::move(settings), logger);

   auto result = client.execute(tbs::http::ClientRequest("PATCH", "/result"));

   ASSERT_TRUE(result.responseReceived());
   EXPECT_EQ(result.response->content(), "/result");
   EXPECT_TRUE(result.error.message.empty());
   client.shutdown();
}

TEST(HttpClientResultTest, SyncConnectionFailureIsReturnedAndNotifiesGlobalHandler)
{
   tbs::log::StdoutLogger logger;
   tbs::http::SettingsClient settings("127.0.0.1", closedPort());
   tbs::http::PlainClientDefault client(std::move(settings), logger);
   std::size_t globalFailures = 0;
   client.onConnectFailed([&](const std::string&) { ++globalFailures; });

   auto result = client.get("/");

   EXPECT_FALSE(result.responseReceived());
   EXPECT_FALSE(result.error.message.empty());
   EXPECT_EQ(result.error.type, tbs::http::ErrorType::system);
   EXPECT_EQ(globalFailures, 1u);
   client.shutdown();
}

TEST(HttpClientResultTest, AsyncConnectionFailureIsReturnedToTheRequestHandler)
{
   tbs::log::StdoutLogger logger;
   tbs::http::SettingsClient settings("127.0.0.1", closedPort());
   tbs::http::PlainClientDefault client(std::move(settings), logger);
   std::size_t globalFailures = 0;
   std::size_t requestFailures = 0;
   tbs::http::ClientResult requestResult;
   client.onConnectFailed([&](const std::string&) { ++globalFailures; });
   client.execute(tbs::http::ClientRequest("CUSTOM", "/"),
      [&](const tbs::http::ClientResult& result)
      {
         requestResult = result;
         ++requestFailures;
         client.shutdown();
      });

   client.run();

   EXPECT_EQ(globalFailures, 1u);
   EXPECT_EQ(requestFailures, 1u);
   EXPECT_FALSE(requestResult);
   EXPECT_FALSE(requestResult.error.message.empty());
   EXPECT_EQ(requestResult.error.type, tbs::http::ErrorType::system);
}

TEST(HttpClientResultTest, SyncConnectionCloseIsReturnedAsRequestError)
{
   TestDisconnectServer server;
   tbs::log::StdoutLogger logger;
   tbs::http::SettingsClient settings("127.0.0.1", server.port());
   tbs::http::PlainClientDefault client(std::move(settings), logger);

   auto result = client.get("/");

   EXPECT_FALSE(result.responseReceived());
   EXPECT_FALSE(result.error.message.empty());
   EXPECT_EQ(result.error.source, "ClientConnection");
   client.shutdown();
}

TEST(HttpClientResultTest, AsyncConnectionCloseIsReturnedAsRequestError)
{
   TestDisconnectServer server;
   tbs::log::StdoutLogger logger;
   tbs::http::SettingsClient settings("127.0.0.1", server.port());
   tbs::http::PlainClientDefault client(std::move(settings), logger);
   std::size_t requestFailures = 0;
   tbs::http::ClientResult requestResult;
   client.get("/",
      [&](const tbs::http::ClientResult& result)
      {
         requestResult = result;
         ++requestFailures;
         client.shutdown();
      });

   client.run();

   EXPECT_EQ(requestFailures, 1u);
   EXPECT_FALSE(requestResult);
   EXPECT_FALSE(requestResult.error.message.empty());
   EXPECT_EQ(requestResult.error.source, "ClientConnection");
}

TEST(HttpClientPoolTest, AsyncShutdownDrainsPooledRequestsAfterFailure)
{
   TestHttpServer server;
   tbs::log::StdoutLogger logger;
   tbs::http::SettingsClient settings("127.0.0.1", server.port());
   settings.connectionPoolSize(2);
   tbs::http::PlainClientDefault client(std::move(settings), logger);

   constexpr std::size_t requestCount = 8;
   std::atomic<std::size_t> completed{0};
   std::atomic<std::size_t> failedRequests{0};
   std::atomic<std::size_t> mismatchedResponses{0};

   for (std::size_t index = 0; index < requestCount; ++index)
   {
      const auto target = index == 0 ? "/disconnect" : "/request-" + std::to_string(index);
      client.get(target, [&, target](const tbs::http::ClientResult& result)
      {
         if (!result.responseReceived())
            ++failedRequests;
         else if (result.response->content() != target)
            ++mismatchedResponses;

         ++completed;
      });
   }

   // Shutdown drains all submitted requests, including those queued behind a failed connection.
   client.shutdown();
   client.run(2);

   EXPECT_EQ(completed.load(), requestCount);
   EXPECT_EQ(failedRequests.load(), 1u);
   EXPECT_EQ(mismatchedResponses.load(), 0u);
   EXPECT_GE(server.connectionCount(), 2u);
}

TEST(HttpClientTlsTest, PeerVerificationUsesDefaultCaPathsWithoutCustomFile)
{
   tbs::log::StdoutLogger logger;
   tbs::http::SettingsClient settings("127.0.0.1", 443);
   settings.verifyPeer(true);

   EXPECT_NO_THROW(
      tbs::http::SecureClientDefault client(std::move(settings), logger));
}

TEST(HttpClientTlsTest, PeerVerificationRejectsConfiguredMissingCaFile)
{
   tbs::log::StdoutLogger logger;
   tbs::http::SettingsClient settings("127.0.0.1", 443);
   settings.verifyPeer(true).caVerificationFile("missing-test-ca.pem");

   EXPECT_THROW(
      tbs::http::SecureClientDefault client(std::move(settings), logger),
      tbs::http::Exception);
}

TEST(HttpClientTlsIntegrationTest, ConnectsToPublicHttpsSite)
{
   const auto runInternetTest = tbs::test_support::loadEnvironmentValue("TOBASA_HTTP_RUN_INTERNET_TEST", std::string(TOBASA_PROJECT_SOURCE_DIR) + "/.env");
   if (runInternetTest != "1")
      GTEST_SKIP() << "Set TOBASA_HTTP_RUN_INTERNET_TEST=1 to enable this internet test";

   const auto sslCertFile = tbs::test_support::loadEnvironmentValue("SSL_CERT_FILE", std::string(TOBASA_PROJECT_SOURCE_DIR) + "/.env");
   std::unique_ptr<tbs::test_support::ScopedEnvironmentVariable> sslCertFileEnvironment;
   if (!sslCertFile.empty())
      sslCertFileEnvironment = std::make_unique<tbs::test_support::ScopedEnvironmentVariable>( "SSL_CERT_FILE", sslCertFile);

   tbs::log::StdoutLogger logger;
   tbs::http::SettingsClient settings("example.com", 443);
   settings
      .verifyPeer(true)
      .timeoutRead(10)
      .timeoutWrite(10);

   tbs::http::SecureClientDefault client(std::move(settings), logger);
   auto result = client.get("/");

   ASSERT_TRUE(result.responseReceived()) << result.error.message;
   EXPECT_GE(result.response->statusCode(), 100u);
   client.shutdown();
}