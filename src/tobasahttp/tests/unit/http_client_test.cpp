#include <gtest/gtest.h>

#include <asio.hpp>
#include <atomic>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

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
            while (std::getline(requestStream, headerLine) && headerLine != "\r")
            {
            }

            if (target == "/disconnect")
            {
               asio::error_code ignored;
               self->_socket.close(ignored);
               return;
            }

            const auto responseContent = method == "HEAD" ? std::string() : target;
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