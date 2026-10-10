#pragma once

#include <sstream>
#include <vector>
#include <functional>
#include "tobasahttp/type_common.h"
#include "tobasahttp/message.h"
#include "tobasahttp/status_codes.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */


/**
 * @brief HTTP response message.
 * 
 */
class ClientResponse : public Message
{
private:
   HttpStatus  _httpStatus         {StatusCode::OK};
   HttpVersion _httpVersion        {HttpVersion::one};
   bool        _completed          { false };
   bool        _useChunkedEncoding { false };

public:
   ClientResponse(HttpVersion httpVersion=HttpVersion::one);
   ~ClientResponse() = default;

   uint16_t statusCode() const;
   std::string statusMessage() const;
   void httpStatus(HttpStatus status);
   void httpStatus(StatusCode code);
   HttpStatus httpStatus();
   size_t contentSize() const;

   HttpVersion httpVersion() const   { return _httpVersion; }
   bool useChunkedEncoding() const   { return _useChunkedEncoding; }
   void useChunkedEncoding(bool val) { _useChunkedEncoding = val; }
};
using ClientResponsePtr = std::shared_ptr<ClientResponse>;
using ClientResponseHandler = std::function<void(const ClientResponsePtr&)>;
using ClientResponseHeadersHandler = std::function<void(const ClientResponsePtr&)>;

/// Result of one client request, containing its response or request error. 
struct ClientResult
{
   /// Parsed response; null when the request failed before receiving one.
   ClientResponsePtr response;
   /// Transport or client error; HTTP error status codes are still responses.
   ErrorData error;

   /// Returns true when a response arrived, regardless of its HTTP status.
   bool responseReceived() const noexcept { return response != nullptr; }
   explicit operator bool() const noexcept { return responseReceived(); }
};

using ClientResultHandler = std::function<void(const ClientResult&)>;
using ClientRequestErrorHandler = std::function<void(const ErrorData&)>;


/** @}*/

} // namespace http
} // namespace tbs
