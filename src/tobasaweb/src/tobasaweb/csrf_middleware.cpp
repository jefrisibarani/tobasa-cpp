#include <algorithm>

#include <tobasa/crypt.h>
#include <tobasa/logger.h>
#include <tobasahttp/multipart.h>
#include <tobasahttp/request.h>
#include "tobasaweb/result.h"
#include "tobasaweb/json_result.h"
#include "tobasaweb/session.h"
#include "tobasaweb/csrf_middleware.h"

namespace tbs {
namespace web {

namespace {

constexpr size_t CSRF_TOKEN_BYTE_LENGTH = 32;

bool isSafeMethod(const std::string& method)
{
   return method == "GET" || method == "HEAD" || method == "OPTIONS";
}

bool constantTimeEquals(const std::string& lhs, const std::string& rhs)
{
   const size_t length = std::max(lhs.size(), rhs.size());
   size_t difference = lhs.size() ^ rhs.size();

   for (size_t index = 0; index < length; ++index)
   {
      const unsigned char left = index < lhs.size() ? static_cast<unsigned char>(lhs[index]) : 0;
      const unsigned char right = index < rhs.size() ? static_cast<unsigned char>(rhs[index]) : 0;
      difference |= static_cast<size_t>(left ^ right);
   }

   return difference == 0;
}

std::string submittedToken(const http::HttpContext& context, const CsrfMiddlewareOption& option)
{
   auto request = context->request();
   auto header = request->headers().findHeader(option.tokenHeaderName);
   if (!header.value.empty())
      return header.value;

   auto multipartBody = request->multipartBody();
   if (multipartBody && !multipartBody->empty())
      return multipartBody->value(option.tokenFieldName);

   auto formBody = request->formBody();
   if (formBody && !formBody->empty())
      return formBody->value(option.tokenFieldName);

   return {};
}

} // namespace

CsrfMiddleware::CsrfMiddleware()
{
   _name = "CsrfMiddleware";
}

void CsrfMiddleware::option(CsrfMiddlewareOption option)
{
   _option = std::move(option);
}

http::RequestStatus CsrfMiddleware::invoke(const http::HttpContext& context)
{
   if ((_option.ignoreHandler && _option.ignoreHandler(context)) ||
       (_option.protectHandler && !_option.protectHandler(context)))
      return next(context);

   const std::string method = context->request()->method();
   const bool safeMethod = isSafeMethod(method);

   const std::string sessionId = context->sessionId();
   if (sessionId.empty())
   {
      if (safeMethod)
         return next(context);

      applyHttpResult(context, http::StatusCode::FORBIDDEN, "Missing CSRF token");
      return http::RequestStatus::handled;
   }

   auto session = Session::get(sessionId);
   if (!session->loaded())
   {
      if (safeMethod)
         return next(context);

      applyHttpResult(context, http::StatusCode::FORBIDDEN, "Missing CSRF token");
      return http::RequestStatus::handled;
   }

   const auto storedToken = session->getData(_option.sessionDataKey);
   const std::string expectedToken = storedToken.is_string() ? storedToken.get<std::string>() : std::string();

   if (safeMethod)
   {
      if (expectedToken.empty())
      {
         try
         {
            const std::string token = crypt::getCryptoRandomBase64Url(CSRF_TOKEN_BYTE_LENGTH);
            if (token.empty())
               throw std::runtime_error("CSRF token generation returned an empty value");

            session->setData(_option.sessionDataKey, token);
         }
         catch (const std::exception& ex)
         {
            Logger::logE("[web] [conn:{}] Failed to create CSRF token: {}", context->connId(), ex.what());
            applyHttpResult(context, http::StatusCode::INTERNAL_SERVER_ERROR, "Unable to initialize request security");
            return http::RequestStatus::handled;
         }
      }

      return next(context);
   }

   if (expectedToken.empty() || !constantTimeEquals(expectedToken, submittedToken(context, _option)))
   {
      applyHttpResult(context, http::StatusCode::FORBIDDEN, "Invalid or missing CSRF token");
      return http::RequestStatus::handled;
   }

   return next(context);
}

void CsrfMiddleware::applyHttpResult(const http::HttpContext& context,
   http::StatusCode statusCode, const std::string& message)
{
   if (_resultBuilder)
   {
      auto result = _resultBuilder(context, statusCode, message);
      result->toResponse(context->response());
   }
   else
   {
      if (util::startsWith(context->request()->path(), "/api"))
      {
         http::JsonResult result(statusCode, message);
         result.toResponse(context->response());
      }
      else
      {
         http::StatusResult result(statusCode, message);
         result.toResponse(context->response());
      }
   }
}

} // namespace web
} // namespace tbs