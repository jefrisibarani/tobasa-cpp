#pragma once

#include <tobasa/json.h>
#include "tobasaweb/result.h"

namespace tbs {
namespace web {

/** Tobasa REST API JSON result.
 *  result  : actual request result (successfull or error)
 *            null, JSON object, JSON Array, string, numeric
 *  code    : result status code
 *            numeric, 200: Success, else considered as error
 *  message : optional message
 *
 * // Error response
 * Sample: ApiResult( Json{}, "Unknown parameter", 400, StatusCode::OK );
 * produce this json, with HTTP Status Code 200
 *   {
 *      "result": {},
 *      "code": 400,
 *      "message": "Unknown parameter"
 *   }
 *
 * // Error response
 * Sample: ApiResult( "Unknown parameter", 400, StatusCode::OK );
 * produce this json, with HTTP Status Code 200
 *   {
 *      "code": 400,
 *      "message": "Unknown parameter"
 *   }
 *
 * // Successfull response
 * Sample: ApiResult()
 * produce this json, with HTTP Status Code 200
 *  {
 *      "code": 200,
 *      "message": "OK"
 *  }
 *
 * // Successfull response
 * Sample:
 * Json data;
 * data["AppName"] = "Tobasa";
 * data["Version"] = "2.0";
 *
 * ApiResult( data )
 * produce this json, with HTTP Status Code 200
 *  {
 *      "result": { "appName": "Tobasa", "version": "2.0" },
 *      "code": 200,
 *      "message": "OK"
 *  }
 */

class ApiResult
   : public http::Result
{
protected:
   Json _jContent;
   int _resultCode {0};

public:
   ApiResult(http::StatusCode statusCode)
      : Result("", "application/json")
   {
      _httpStatus.code(statusCode);
      _resultCode = static_cast<int>(statusCode);
   }

   ApiResult(
      const std::string& message,
      int resultCode,
      http::StatusCode statusCode=http::StatusCode::OK)
      : http::Result(message, "application/json")
   {
      _httpStatus.code(statusCode);
      _resultCode = resultCode;
   }

   ApiResult(
      const Json& result,
      const std::string& message="OK",
      int resultCode=200,
      http::StatusCode statusCode=http::StatusCode::OK)
      : http::Result(message, "application/json")
   {
      _httpStatus.code(statusCode);
      _resultCode = resultCode;
      _jContent["result"] = result;
   }

   virtual std::string className() { return "web::ApiResult"; }

   virtual std::string buildContent()
   {
      if (_content.empty())
         _jContent["message"]  = _httpStatus.reason();
      else
         _jContent["message"]  = _content;

      _jContent["code"] = _resultCode;

      return _jContent.dump();
   }
   
   Json& jsonContent()
   {
      return _jContent;
   }
   
   int resultCode()
   {
      return _resultCode;
   }

};

template <typename... Params>
[[nodiscard]]
std::shared_ptr<http::Result>
apiResult(Params&&... args)
{
   return std::static_pointer_cast<http::Result>(
      std::make_shared<ApiResult>(std::forward<Params>(args)...)
   );
}

// Helper functions
// -------------------------------------------------------
[[nodiscard]]
std::shared_ptr<http::Result>
inline okResult(const std::string& message="OK", int resultCode=200, http::StatusCode statusCode=http::StatusCode::OK)
{
   return std::static_pointer_cast<http::Result>(
      std::make_shared<ApiResult>(message, resultCode, statusCode)
   );
}

[[nodiscard]]
std::shared_ptr<http::Result>
inline object(const Json& result, const std::string& message="OK", int resultCode=200, http::StatusCode statusCode=http::StatusCode::OK)
{
   return std::static_pointer_cast<http::Result>(
      std::make_shared<ApiResult>(result, message, resultCode, statusCode)
   );
}

[[nodiscard]]
std::shared_ptr<http::Result>
inline badParameter(const std::string& message, int resultCode=201, http::StatusCode statusCode=http::StatusCode::OK)
{
   return std::static_pointer_cast<http::Result>(
      std::make_shared<ApiResult>(message, resultCode, statusCode)
   );
}

[[nodiscard]]
std::shared_ptr<http::Result>
inline failed(const std::string& message, int resultCode=400, http::StatusCode statusCode=http::StatusCode::BAD_REQUEST)
{
   return std::static_pointer_cast<http::Result>(
      std::make_shared<ApiResult>(message, resultCode, statusCode)
   );
}

[[nodiscard]]
std::shared_ptr<http::Result>
inline notFound(const std::string& message, int resultCode=404, http::StatusCode statusCode=http::StatusCode::NOT_FOUND)
{
   return std::static_pointer_cast<http::Result>(
      std::make_shared<ApiResult>(message, resultCode, statusCode)
   );
}

/// @brief Authentication is missing or invalid
[[nodiscard]]
std::shared_ptr<http::Result>
inline unauthorized(const std::string& message="Unauthorized", int resultCode=401, http::StatusCode statusCode=http::StatusCode::UNAUTHORIZED)
{
   return std::static_pointer_cast<http::Result>(
      std::make_shared<ApiResult>(message, resultCode, statusCode)
   );
}

/// @brief Authentication is valid, but access is denied
[[nodiscard]]
std::shared_ptr<http::Result>
inline forbidden(const std::string& message="Forbidden", int resultCode=403, http::StatusCode statusCode=http::StatusCode::FORBIDDEN)
{
   return std::static_pointer_cast<http::Result>(
      std::make_shared<ApiResult>(message, resultCode, statusCode)
   );
}

[[nodiscard]]
std::shared_ptr<http::Result>
inline badRequest(const std::string& message="Bad request", int resultCode=400, http::StatusCode statusCode=http::StatusCode::BAD_REQUEST)
{
   return std::static_pointer_cast<http::Result>(
      std::make_shared<ApiResult>(message, resultCode, statusCode)
   );
}

[[nodiscard]]
std::shared_ptr<http::Result>
inline notImplemented(const std::string& message="Not Implemented", int resultCode=501, http::StatusCode statusCode=http::StatusCode::NOT_IMPLEMENTED)
{
   return std::static_pointer_cast<http::Result>(
      std::make_shared<ApiResult>(message, resultCode, statusCode)
   );
}

[[nodiscard]]
std::shared_ptr<http::Result>
inline appError(const std::string& message="", int resultCode=500, http::StatusCode statusCode=http::StatusCode::INTERNAL_SERVER_ERROR)
{
   return std::static_pointer_cast<http::Result>(
      std::make_shared<ApiResult>(message, resultCode, statusCode)
   );
}

} // namespace web
} // namespace tbs