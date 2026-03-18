#include "tobasahttp/message.h"

namespace tbs {
namespace http {

std::string Message::contentType()
{
   return _headers.value("content-type");
}

size_t Message::contentLength()
{
   auto length = _headers.value("content-length");
   if (length.empty()) // content-length header not found
      return 0;
   else
      return std::stoull(length);
}

void Message::majorVersion(uint16_t value)
{
   _majorVersion = value;
}

void Message::minorVersion(uint16_t value)
{
   _minorVersion = value;
}

void Message::headers(Headers headers)
{
   _headers = std::move(headers);
}

void Message::content(const std::string& content)
{
   _content = content;
}

void Message::setHeaderContentType(const std::string& value)
{
   setHeader("Content-Type", value);
}

void Message::setHeaderContentLength(size_t length)
{
   setHeader("Content-Length", std::to_string(length));
}

void Message::addHeader(const std::string& name, const std::string& value)
{
   _headers.add(name, value);
}

void Message::setHeader(const std::string& name, const std::string& value)
{
   _headers.set(name, value);
}

} // namespace http
} // namespace tbs