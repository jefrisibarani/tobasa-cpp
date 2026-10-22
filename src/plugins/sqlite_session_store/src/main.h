#pragma once

#include <memory>
#include <string>
#include <tobasaweb/session_store_plugin.h>

namespace tbs {

namespace web {
   class ISessionStore;
}

/**
 * @brief Sqlite Session Store
 * 
 */
class SqliteSessionStorePlugin : public web::SessionStorePlugin
{
public:
   SqliteSessionStorePlugin() = default;
   ~SqliteSessionStorePlugin() override = default;

   /// Called when the plugin is loaded
   void load() override;

   /// Called when the plugin is unloaded
   void unload() override;

   /// Get plugin information
   std::string getInfo() const;

   std::unique_ptr<web::ISessionStore> createStore(const web::SessionStoreConfig& config) const override;

private:
   std::string _info {"SqliteSessionStorePlugin is loaded"};
};

} // namespace tbs