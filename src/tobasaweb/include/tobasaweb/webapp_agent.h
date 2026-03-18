#pragma once

#include <string>
#include <memory>
#include <tobasahttp/server/http_server.h>
#include "tobasaweb/webapp_status.h"

namespace tbs {
namespace web {

class Webapp;

class WebappAgent
{
   friend class Webapp;

public:

   void updateStatus();
   WebappStatus status();

private:

   Webapp* _pApp                   = nullptr;
   http::PlainServer* _pSrvPlain   = nullptr;
   http::SecureServer* _pSrvSecure = nullptr;

   WebappStatus _status;
};

} // namespace web
} // namespace tbs