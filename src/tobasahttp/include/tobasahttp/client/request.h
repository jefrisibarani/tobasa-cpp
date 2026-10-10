#pragma once

#include <string>
#include <vector>
#include <utility>
#include <functional>
#include <cstdint>
#include "tobasahttp/type_common.h"
#include "tobasahttp/message.h"
#include "tobasahttp/client/request_data_source.h"
#include "tobasahttp/client/response.h"

namespace tbs {
namespace http {

using ClientResponseBodyHandler = std::function<void(const uint8_t *data, size_t size)>;

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
   ClientResponseHeadersHandler _responseHeadersHandler {};
   ClientResponseBodyHandler _responseBodyHandler {};
   std::shared_ptr<RequestDataSource> _bodySource {};

public:
   using Message::content;

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

   /// Sets a synchronous handler called after response headers are validated and before body delivery.
   void responseHeadersHandler(ClientResponseHeadersHandler handler);
   /// Returns the response-headers handler, if configured.
   const ClientResponseHeadersHandler& responseHeadersHandler() const noexcept;

   /// Sets a synchronous handler for decoded response-body chunks.
   void responseBodyHandler(ClientResponseBodyHandler handler);
   /// Returns the response-body handler, if configured.
   const ClientResponseBodyHandler& responseBodyHandler() const noexcept;

   /// Sets an owned, known-size source for the outbound request body.
   void bodySource(std::shared_ptr<RequestDataSource> source);
   /// Returns the outbound body source, if configured.
   const std::shared_ptr<RequestDataSource>& bodySource() const noexcept;

   /// Replaces any body source with a buffered request body.
   void content(const std::string& content) override;
};


/** @}*/

} // namespace http
} // namespace tbs
