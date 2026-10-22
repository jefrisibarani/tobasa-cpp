#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#include <tobasa/config.h>
#include <tobasahttp/request.h>
#include <tobasahttp/response.h>
#include <tobasaweb/csrf_middleware.h>
#include <tobasaweb/session.h>
#include <tobasaweb/settings_webapp.h>

namespace {

tbs::http::HttpContext makeContext(
   const std::string& method,
   const std::string& path,
   const std::string& sessionId={})
{
   auto request = std::make_shared<tbs::http::Request>(tbs::http::HttpVersion::one);
   request->method(method);
   request->path(path);
   request->sessionId(sessionId);

   auto response = std::make_shared<tbs::http::Response>(tbs::http::HttpVersion::one);
   return std::make_shared<tbs::http::Context>(
      request,
      response,
      asio::ip::tcp::endpoint{},
      0,
      0,
      tbs::http::HttpVersion::one
   #ifdef TOBASA_HTTP_USE_HTTP2
      , -1
   #endif
      );
}

void configureTestSessions()
{
   static bool configured = false;
   if (configured)
      return;

   const auto sessionDirectory = std::filesystem::temp_directory_path() / "tobasa_csrf_middleware_tests";
   std::filesystem::create_directories(sessionDirectory);

   tbs::web::conf::Webapp webappSettings;
   webappSettings.dbConnectionPoolSize = 1;
   webappSettings.webService.sessionSavePath = sessionDirectory.string();

   tbs::Json configuration;
   configuration["webapp"] = webappSettings;
   const std::string configurationText = configuration.dump();
   tbs::span<const unsigned char> configurationData(
      reinterpret_cast<const unsigned char*>(configurationText.data()),
      configurationText.size());
   tbs::Config::get().load("appsettings.json", configurationData);
   configured = true;
}

void storeCsrfToken(const std::string& sessionId, const std::string& token)
{
   auto session = tbs::web::Session::create(sessionId);
   session->setData("csrf_token", token);
}

} // namespace

TEST(CsrfMiddlewareTest, AllowsSafeRequestWithoutSession)
{
   auto context = makeContext("GET", "/login");
   bool nextCalled = false;

   tbs::web::CsrfMiddleware middleware;
   middleware.nextHandler([&nextCalled](const tbs::http::HttpContext&) {
      nextCalled = true;
      return tbs::http::RequestStatus::handled;
   });

   EXPECT_EQ(middleware.invoke(context), tbs::http::RequestStatus::handled);
   EXPECT_TRUE(nextCalled);
}

TEST(CsrfMiddlewareTest, RejectsUnsafeRequestWithoutSessionOrToken)
{
   auto context = makeContext("POST", "/login");
   bool nextCalled = false;

   tbs::web::CsrfMiddleware middleware;
   middleware.nextHandler([&nextCalled](const tbs::http::HttpContext&) {
      nextCalled = true;
      return tbs::http::RequestStatus::handled;
   });

   EXPECT_EQ(middleware.invoke(context), tbs::http::RequestStatus::handled);
   EXPECT_FALSE(nextCalled);
   EXPECT_EQ(context->response()->statusCode(), 403);
}

TEST(CsrfMiddlewareTest, SkipsRequestsOutsideConfiguredPolicy)
{
   auto context = makeContext("POST", "/api/users/update");
   bool nextCalled = false;

   tbs::web::CsrfMiddleware middleware;
   tbs::web::CsrfMiddlewareOption option;
   option.protectHandler = [](const tbs::http::HttpContext&) { return false; };
   middleware.option(std::move(option));
   middleware.nextHandler([&nextCalled](const tbs::http::HttpContext&) {
      nextCalled = true;
      return tbs::http::RequestStatus::handled;
   });

   EXPECT_EQ(middleware.invoke(context), tbs::http::RequestStatus::handled);
   EXPECT_TRUE(nextCalled);
}

TEST(CsrfMiddlewareTest, AcceptsValidHeaderToken)
{
   configureTestSessions();
   const std::string sessionId = "11111111-1111-4111-8111-111111111111";
   const std::string token = "valid_csrf_token";
   storeCsrfToken(sessionId, token);

   auto context = makeContext("POST", "/login", sessionId);
   context->request()->headers().set("X-CSRF-Token", token);
   bool nextCalled = false;

   tbs::web::CsrfMiddleware middleware;
   middleware.nextHandler([&nextCalled](const tbs::http::HttpContext&) {
      nextCalled = true;
      return tbs::http::RequestStatus::handled;
   });

   EXPECT_EQ(middleware.invoke(context), tbs::http::RequestStatus::handled);
   EXPECT_TRUE(nextCalled);
   EXPECT_TRUE(tbs::web::Session::destroy(sessionId));
}

TEST(CsrfMiddlewareTest, CreatesTokenForSafeRequestWithSession)
{
   configureTestSessions();
   const std::string sessionId = "55555555-5555-4555-8555-555555555555";
   tbs::web::Session::create(sessionId);

   auto context = makeContext("GET", "/login", sessionId);
   bool nextCalled = false;

   tbs::web::CsrfMiddleware middleware;
   middleware.nextHandler([&nextCalled](const tbs::http::HttpContext&) {
      nextCalled = true;
      return tbs::http::RequestStatus::handled;
   });

   EXPECT_EQ(middleware.invoke(context), tbs::http::RequestStatus::handled);
   EXPECT_TRUE(nextCalled);
   const auto token = tbs::web::Session::get(sessionId)->getData("csrf_token");
   EXPECT_TRUE(token.is_string());
   EXPECT_FALSE(token.get<std::string>().empty());
   EXPECT_TRUE(tbs::web::Session::destroy(sessionId));
}

TEST(CsrfMiddlewareTest, AcceptsValidFormToken)
{
   configureTestSessions();
   const std::string sessionId = "22222222-2222-4222-8222-222222222222";
   const std::string token = "valid_form_token";
   storeCsrfToken(sessionId, token);

   auto context = makeContext("POST", "/register", sessionId);
   context->request()->setHeaderContentType("application/x-www-form-urlencoded");
   context->request()->content("_csrf=" + token);
   bool nextCalled = false;

   tbs::web::CsrfMiddleware middleware;
   middleware.nextHandler([&nextCalled](const tbs::http::HttpContext&) {
      nextCalled = true;
      return tbs::http::RequestStatus::handled;
   });

   EXPECT_EQ(middleware.invoke(context), tbs::http::RequestStatus::handled);
   EXPECT_TRUE(nextCalled);
   EXPECT_TRUE(tbs::web::Session::destroy(sessionId));
}

TEST(CsrfMiddlewareTest, RejectsTokenFromDifferentSession)
{
   configureTestSessions();
   const std::string sessionId = "33333333-3333-4333-8333-333333333333";
   const std::string otherSessionId = "44444444-4444-4444-8444-444444444444";
   const std::string token = "first_session_token";
   storeCsrfToken(sessionId, token);
   storeCsrfToken(otherSessionId, "second_session_token");

   auto context = makeContext("POST", "/password", otherSessionId);
   context->request()->headers().set("X-CSRF-Token", token);
   bool nextCalled = false;

   tbs::web::CsrfMiddleware middleware;
   middleware.nextHandler([&nextCalled](const tbs::http::HttpContext&) {
      nextCalled = true;
      return tbs::http::RequestStatus::handled;
   });

   EXPECT_EQ(middleware.invoke(context), tbs::http::RequestStatus::handled);
   EXPECT_FALSE(nextCalled);
   EXPECT_EQ(context->response()->statusCode(), 403);
   EXPECT_TRUE(tbs::web::Session::destroy(sessionId));
   EXPECT_TRUE(tbs::web::Session::destroy(otherSessionId));
}

TEST(CsrfMiddlewareTest, RejectsMissingAndInvalidTokenForExistingSession)
{
   configureTestSessions();
   const std::string sessionId = "66666666-6666-4666-8666-666666666666";
   storeCsrfToken(sessionId, "expected_token");

   bool nextCalled = false;
   tbs::web::CsrfMiddleware middleware;
   middleware.nextHandler([&nextCalled](const tbs::http::HttpContext&) {
      nextCalled = true;
      return tbs::http::RequestStatus::handled;
   });

   auto missingTokenContext = makeContext("POST", "/password", sessionId);
   EXPECT_EQ(middleware.invoke(missingTokenContext), tbs::http::RequestStatus::handled);
   EXPECT_FALSE(nextCalled);
   EXPECT_EQ(missingTokenContext->response()->statusCode(), 403);

   auto invalidTokenContext = makeContext("POST", "/password", sessionId);
   invalidTokenContext->request()->headers().set("X-CSRF-Token", "wrong_token");
   EXPECT_EQ(middleware.invoke(invalidTokenContext), tbs::http::RequestStatus::handled);
   EXPECT_FALSE(nextCalled);
   EXPECT_EQ(invalidTokenContext->response()->statusCode(), 403);
   EXPECT_TRUE(tbs::web::Session::destroy(sessionId));
}