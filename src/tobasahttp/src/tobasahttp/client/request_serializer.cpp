#include <sstream>
#include <stdexcept>

#include "tobasahttp/client/request.h"
#include "tobasahttp/client/request_serializer.h"

namespace tbs {
namespace http {

RequestSerializer::RequestSerializer(ClientRequest& request)
   : _request(request)
{}

std::string RequestSerializer::getHeaderString()
{
   const auto& content = _request.content();
   if (_request.bodySource())
   {
      if (_request.headers().hasField("Transfer-Encoding"))
         throw std::logic_error("Source-backed requests use Content-Length framing");

      _request.setHeaderContentLength(_request.bodySource()->size());
   }
   else if (!content.empty())
      _request.addHeader("Content-Length", std::to_string(content.size()));

   std::stringstream stream;
   stream << _request.method()
          << " "
          << _request.target()
          << " HTTP/"
          << _request.majorVersion()
          << "."
          << _request.minorVersion()
          << "\r\n";

   auto& headers = _request.headers();
   for (size_t index = 0; index < headers.size(); ++index)
   {
      auto field = headers.field(index);
      if (field != nullptr)
         stream << field->name() << ": " << field->value() << "\r\n";
   }

   stream << "\r\n";

   return stream.str();
}

std::string RequestSerializer::getString()
{
   auto request = getHeaderString();
   if (!_request.bodySource() && !_request.content().empty())
      request += _request.content();

   return request;
}

} // namespace http
} // namespace tbs