#pragma once

#include <string>

namespace tbs {
namespace http {

class ClientRequest;

/** Serializes an outbound client request as an HTTP/1.x message. */
class RequestSerializer
{
private:
   ClientRequest& _request;

public:
   explicit RequestSerializer(ClientRequest& request);
   std::string getHeaderString();
   std::string getString();
};

} // namespace http
} // namespace tbs