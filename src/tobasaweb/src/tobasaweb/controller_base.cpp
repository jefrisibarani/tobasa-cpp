#include "tobasaweb/result.h"
#include "tobasaweb/controller_base.h"

namespace tbs {
namespace web {

http::ResultPtr ControllerBase::redirect(const std::string& location, http::StatusCode statusCode)
{
   auto result = http::makeResult();
   result->contentType("");
   result->redirect(location);
   
   //default to http::StatusCode::FOUND
   result->statusCode(statusCode); 

   return std::move(result);
}

} // namespace http
} // namespace tbs
