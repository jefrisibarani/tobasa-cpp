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
 * @brief HTTP client request message.
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
   ClientRequest(const std::string&, const std::string& target, HttpVersion version=HttpVersion::one);
   ~ClientRequest() = default;

   void method(const std::string& val);
   std::string method() const;

   void target(const std::string& val);
   std::string target() const;
};


/** @}*/

} // namespace http
} // namespace tbs
