#include "tobasaweb/webapp_agent.h"
#include "tobasaweb/webapp.h"

namespace {
#ifdef _WIN32
   #include <windows.h>
   long getProcessId() 
   {
      return static_cast<long>(GetCurrentProcessId());
   }
#else
   #include <unistd.h>
   long getProcessId() 
   {
      return static_cast<long>(getpid());
   }
#endif
}

namespace tbs {
namespace web {

void WebappAgent::updateStatus()
{
   _status.processId      = getProcessId();
   _status.threadpoolSize = static_cast<long long>(_pApp->_ioPoolSize);
   _status.dbConnected    = _pApp->dbConnected();

   _status.webappThreadIds = {};
   for (auto &t: _pApp->_threadPool)
   {
      std::thread::id threadId = t.get_id();
      _status.webappThreadIds.emplace_back(util::threadId(threadId));
   }

   if (_pSrvPlain != nullptr && _pSrvSecure != nullptr)
   {
      _status.totalHttpConnection  = static_cast<long long>(_pSrvPlain->totalConnections());
      _status.totalHttpsConnection = static_cast<long long>(_pSrvSecure->totalConnections());

      _status.lastHttpConnectionId  = _pSrvPlain->lastConnectionId();
      _status.lastHttpsConnectionId = _pSrvSecure->lastConnectionId();


      _status.currentConnections = {};
      for (auto& conn: _pSrvPlain->currentConnectionsInfo())
      {
         _status.currentConnections.push_back(conn);
      }

      for (auto& conn: _pSrvSecure->currentConnectionsInfo())
      {
         _status.currentConnections.push_back(conn);
      }
   }
}

WebappStatus WebappAgent::status()
{
   return _status;
}

} // namespace web
} // namespace tbs