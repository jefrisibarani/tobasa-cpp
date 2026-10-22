#pragma once

#include <string>
#include "tobasahttp/exp_imp.h"

namespace tbs {
namespace http {

class ClientRequest;

/** Serializes an outbound client request as an HTTP/1.x message. */
class TBS_HTTP_API RequestSerializer
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