#pragma once

#include "tobasaweb/exp_imp.h"

#include <memory>
#include <string>
#include <tobasa/json.h>

namespace tbs {
namespace web {

enum class SessionStoreStatus
{
   ok,
   not_found,
   expired,
   invalid_session_id,
   invalid_payload,
   io_error,
   serialization_error,
   shutdown,
   unavailable
};

struct SessionStoreResult
{
   SessionStoreStatus status {SessionStoreStatus::ok};
   Json value;
   std::string message;
};

class TBS_WEB_API ISessionStore
{
public:
   virtual ~ISessionStore() = default;

   virtual SessionStoreResult load(const std::string& sessionId) = 0;
   virtual bool save(const std::string& sessionId, const Json& value, long long expiresAtUtcSeconds) = 0;
   virtual bool remove(const std::string& sessionId) = 0;
   virtual bool cleanupExpired(long long nowUtcSeconds) = 0;
   virtual void shutdown() = 0;
};

TBS_WEB_API bool isValidSessionId(const std::string& sessionId);

class TBS_WEB_API FileSessionStore : public ISessionStore
{
public:
   explicit FileSessionStore(const std::string& sessionRootPath = "");
   ~FileSessionStore() override;

   SessionStoreResult load(const std::string& sessionId) override;
   bool save(const std::string& sessionId, const Json& value, long long expiresAtUtcSeconds) override;
   bool remove(const std::string& sessionId) override;
   bool cleanupExpired(long long nowUtcSeconds) override;
   void shutdown() override;

   std::string sessionRootPath() const;

private:
   std::string sessionPathFor(const std::string& sessionId) const;
   std::string _sessionRootPath;
};

// std::unique_ptr<ISessionStore> makeSessionStore(
//    const std::string& backendName,
//    const std::string& sessionRootPath = "",
//    const std::string& databasePath = "");

} // namespace web
} // namespace tbs
