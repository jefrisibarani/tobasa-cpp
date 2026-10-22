#include <tobasa/exp_imp.h>
#include <tobasa/logger.h>
#include <tobasa/config.h>
#include <tobasaweb/settings_webapp.h>

#include <iostream>
#include "sqlite_session_store.h"
#include "main.h"

namespace tbs {

/// Creates and manages SqliteSessionStorePlugin instances. This factory is retrieved
/// by the PluginManager when the plugin library is loaded.
static PluginFactory<SqliteSessionStorePlugin> sqliteSessionStorePluginFactory(
   "Session SQLite Plugin",
   "session_store_sqlite_plugin",
   "Tobasa",
   "1.0.0",
   "SQLite-backed session store plugin",
   PluginType::addon);

/// Export function for plugin loading
/// This function must be exported from the plugin DLL/SO and is called
/// by PluginLibrary to retrieve the plugin factory.
TBS_PLUGIN_API IPluginFactory* GetTobasaPlugin()
{
   return &sqliteSessionStorePluginFactory;
}

void SqliteSessionStorePlugin::load()
{
   Logger::logI("SqliteSessionStorePlugin: load() called");
   std::cout << "SqliteSessionStorePlugin loaded successfully!" << std::endl;
}

void SqliteSessionStorePlugin::unload()
{
   Logger::logI("SqliteSessionStorePlugin: unload() called");
   std::cout << "SqliteSessionStorePlugin unloaded successfully!" << std::endl;
}

std::string SqliteSessionStorePlugin::getInfo() const
{
   return _info;
}

std::unique_ptr<web::ISessionStore> SqliteSessionStorePlugin::createStore(const web::SessionStoreConfig& config) const
{
   std::string dbPath = config.appDataDir + path::SEPARATOR + "session_store.db";
   return std::make_unique<web::SqliteSessionStore>(dbPath);
}


} // namespace tbs
