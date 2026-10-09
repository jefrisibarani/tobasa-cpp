#pragma once

#include <string>
#include <vector>
#include <utility>
#include "tobasahttp/type_common.h"
#include "tobasahttp/message.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */


/**
 * @brief Outbound HTTP client request message.
 * 
 */
class ClientRequest : public Message
{
private:
   std::string _method      {};
   std::string _target      {};
   HttpVersion _httpVersion {HttpVersion::one};

public:
   ClientRequest(HttpVersion version=HttpVersion::one);
   ClientRequest(const std::string& method, const std::string& target, HttpVersion version=HttpVersion::one);
   ~ClientRequest() = default;

   /// Sets the request method.
   void method(const std::string& val);
   /// Returns the request method.
   std::string method() const;

   /// Sets the complete outbound request target, including its query string.
   void target(const std::string& val);
   /// Returns the complete outbound request target.
   std::string target() const;

   /// Sets the HTTP version metadata for this request.
   void httpVersion(HttpVersion version) noexcept;
   /// Returns the HTTP version metadata for this request.
   HttpVersion httpVersion() const noexcept;
};


/** @}*/

} // namespace http
} // namespace tbs
