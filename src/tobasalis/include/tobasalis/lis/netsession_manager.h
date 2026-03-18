#pragma once

#include <set>
#include <tobasa/non_copyable.h>
#include "tobasalis/lis/netsession.h"

namespace tbs {
namespace lis {

/** \ingroup LIS
 * NetsessionManager
 * Manages open connections so that they may be cleanly stopped when the server
 * needs to shut down.
 */
class NetsessionManager : private NonCopyable
{
public:
   NetsessionManager();

   /// Add the specified session to the manager.
   NetSessionPtr addListener(NetSessionPtr session);

   /// Add the specified session to the manager and start it.
   NetSessionPtr addClient(NetSessionPtr session);

   /// Stop the specified session.
   void stop(NetSessionPtr session, const std::string& message="");

   /// Stop all session.
   void stopAll();

private:
   /// The managed connections in server mode
   std::set<NetSessionPtr> _sessions;
};

} // namespace lis
} // namespace tbs 