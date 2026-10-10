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

void ClientRequest::httpVersion(HttpVersion version) noexcept
{
   _httpVersion = version;
}

HttpVersion ClientRequest::httpVersion() const noexcept
{
   return _httpVersion;
}

void ClientRequest::responseHeadersHandler(ClientResponseHeadersHandler handler)
{
   _responseHeadersHandler = std::move(handler);
}

const ClientResponseHeadersHandler& ClientRequest::responseHeadersHandler() const noexcept
{
   return _responseHeadersHandler;
}

void ClientRequest::responseBodyHandler(ClientResponseBodyHandler handler)
{
   _responseBodyHandler = std::move(handler);
}

const ClientResponseBodyHandler& ClientRequest::responseBodyHandler() const noexcept
{
   return _responseBodyHandler;
}

void ClientRequest::bodySource(std::shared_ptr<RequestDataSource> source)
{
   _bodySource = std::move(source);
   Message::content({});
}

const std::shared_ptr<RequestDataSource>& ClientRequest::bodySource() const noexcept
{
   return _bodySource;
}

void ClientRequest::content(const std::string& content)
{
   _bodySource.reset();
   Message::content(content);
}

} // namespace http
} // namespace tbs
