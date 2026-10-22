#pragma once
#include <tobasaweb/session_store.h>

namespace tbs {
namespace web {

class SqliteSessionStore : public ISessionStore
{
public:
   explicit SqliteSessionStore(const std::string& databasePath = "");
   ~SqliteSessionStore() override;

   SessionStoreResult load(const std::string& sessionId) override;
   bool save(const std::string& sessionId, const Json& value, long long expiresAtUtcSeconds) override;
   bool remove(const std::string& sessionId) override;
   bool cleanupExpired(long long nowUtcSeconds) override;
   void shutdown() override;

private:
   bool initializeDatabase();
   bool isOpen() const;

   std::string _dbPath;
   void* _db {nullptr};
};

} // namespace web
} // namespace tbs