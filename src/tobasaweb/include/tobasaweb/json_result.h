#pragma once

#include <tobasa/json.h>
#include "tobasaweb/result.h"

namespace tbs {
namespace http {

/** \addtogroup WEB
 * @{
 */

/** 
 * JsonResult

   example #1:
   {
      "result": {},
      "code": 404,
      "message": "Not Found"
   }

   example #2:
   {
      "code": 404,
      "message": "Not Found"
   }

 */
class JsonResult
   : public Result
{
protected:
   Json _jContent;

public:
   JsonResult()
      : Result()
   {
      _contentType = "application/json";
   }

   JsonResult(StatusCode statusCode, const std::string& message={})
      : Result(message, "application/json")
   {
      _httpStatus.code(statusCode);
   }

   JsonResult(const Json& result,
      StatusCode statusCode=StatusCode::OK,
      const std::string& message={})
      : Result(message, "application/json")
   {
      _httpStatus.code(statusCode);
      _jContent["result"] = result;
   }

   virtual std::string className() { return "http::JsonResult"; }

   virtual std::string buildContent()
   {
      if (_content.empty())
         _jContent["message"]  = _httpStatus.reason();
      else
         _jContent["message"]  = _content;


      _jContent["code"] = static_cast<unsigned int>(_httpStatus.code());

      return _jContent.dump();
   }

};

template <typename... Params>
[[nodiscard]]
std::shared_ptr<Result> jsonResult(Params&&... args)
{
   return std::static_pointer_cast<Result>(
      std::make_shared<JsonResult>(std::forward<Params>(args)...)
   );
}

/** @}*/

} // namespace http
} // namespace tbs