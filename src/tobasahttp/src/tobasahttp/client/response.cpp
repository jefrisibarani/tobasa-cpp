#include "tobasahttp/client/response.h"

namespace tbs {
namespace http {

ClientResponse::ClientResponse(HttpVersion httpVersion)
   : Message()
   , _httpVersion {httpVersion}
{}


uint16_t ClientResponse::statusCode() const
{
   return static_cast<uint16_t>(_httpStatus.code());
}

std::string ClientResponse::statusMessage() const
{
   return _httpStatus.reason();
}

void ClientResponse::httpStatus(HttpStatus status)
{
   _httpStatus = status;
}

void ClientResponse::httpStatus(StatusCode code)
{
   _httpStatus = HttpStatus(code);
}

HttpStatus ClientResponse::httpStatus()
{
   return _httpStatus;
}

size_t ClientResponse::contentSize() const
{
   return _content.size();
}

} // namespace http
} // namespace tbs
