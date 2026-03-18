#pragma once

#include <string>
#include <tobasa/json.h>

namespace tbs {
namespace lis {

struct EngineError
{
   std::string task;
   std::string message;
   long long timestamp = 0;
   std::string source;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EngineError, task, message, timestamp, source)

class EngineState
{
public:
   std::string threadId;
   std::string appThreadId;
   std::string lisHostId;
   std::string lisHostProvider;
   std::string lisRole;
   std::string lisStartupMode;
   std::string lisInstrument;
   std::string lisRunningState  = "N/A";
   std::string lisTcpMode       = "N/A";
   std::string lisTCPConnState  = "N/A";
   std::string lisLinkState     = "N/A";
   long        totalRequestMessageReceived;
   long        totalOrderMessageReceivedACK;
   long        totalMessageReceived;
   std::string appcompilerInfo;
   std::string appNameVersion;
   std::string appBuildDate;
   std::string appDbInfo;
   std::string lisDbInfo;
   std::string lisUptime;
   std::string lisDownTime;
   std::vector<EngineError> lisErrors;
};


} // namespace lis
} // namespace tbs