#include <set>
#include <tobasa/logger.h>
#include "tobasalis/lis/netsession_manager.h"

namespace tbs {
namespace lis {

NetsessionManager::NetsessionManager() {}

NetSessionPtr NetsessionManager::addListener(NetSessionPtr session)
{
   session->setListener();
   _sessions.insert(session);
   return session;
}

NetSessionPtr NetsessionManager::addClient(NetSessionPtr session)
{
   _sessions.insert(session);
   return session;
}

void NetsessionManager::stop(NetSessionPtr session, const std::string& errMessage)
{
   if (!errMessage.empty())
   {
      Logger::logE("[lis_link] Stopping session, reason: {}", errMessage);
   }
   _sessions.erase(session);
   session->close();
}

void NetsessionManager::stopAll()
{
   for (auto s: _sessions) {
      s->close();
   }

   _sessions.clear();
}

} // namespace lis
} // namespace tbs