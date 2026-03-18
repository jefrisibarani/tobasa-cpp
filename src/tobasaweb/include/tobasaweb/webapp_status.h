#pragma once

#include <string>
#include <tobasa/json.h>
#include <tobasa/datetime.h>
#include <tobasahttp/type_common.h>

namespace tbs {
namespace web {

struct webappError
{
   std::string task;
   std::string message;
   long long   timestamp = 0;
   std::string source;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(webappError, task, message, timestamp, source)

class WebappStatus
{
public:
   long      processId;
   bool      httpsOnly;
   int       httpPort;
   int       httpsPort;
   long long threadpoolSize;
   bool      dbConnected;
   long long totalHttpConnection;
   long long totalHttpsConnection;
   long long lastHttpConnectionId;
   long long lastHttpsConnectionId;

   std::string               appThreadId;
   std::vector<std::string>  webappThreadIds;
   std::shared_ptr<DateTime> startedTime;

   std::vector<http::ConnectionInfo> currentConnections;

   std::vector<webappError>  errors;
};

} // namespace web
} // namespace tbs