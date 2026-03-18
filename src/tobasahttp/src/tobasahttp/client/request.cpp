#include "tobasahttp/client/request.h"

namespace tbs {
namespace http {

ClientRequest::ClientRequest(HttpVersion version)
   : Message()
   , _httpVersion {version}
{}

ClientRequest::ClientRequest(const std::string& method, const std::string& target, HttpVersion version)
   : Message()
   , _method {method}
   , _target {target}
   , _httpVersion {version}
{}


void ClientRequest::method(const std::string& val)
{
   _method = val;
}

std::string ClientRequest::method() const
{
   return _method;
}

void ClientRequest::target(const std::string& val)
{
   _target = val;
}

std::string ClientRequest::target() const
{
   return _target;
}

} // namespace http
} // namespace tbs
