#include <sstream>

#include "tobasahttp/client/request.h"
#include "tobasahttp/client/request_serializer.h"

namespace tbs {
namespace http {

RequestSerializer::RequestSerializer(ClientRequest& request)
   : _request(request)
{}

std::string RequestSerializer::getString()
{
   const auto& content = _request.content();
   if (!content.empty())
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
   if (!content.empty())
      stream << content;

   return stream.str();
}

} // namespace http
} // namespace tbs