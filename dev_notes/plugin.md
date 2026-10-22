# Tobasa Plugin System

Tobasa plugins are shared libraries that export a factory with the C symbol
`GetTobasaPlugin`. The factory supplies plugin metadata and creates a `Plugin`
instance. The host uses `PluginManager` to load libraries and control plugin
lifetime.

## Implement a Plugin

A plugin derives from `tbs::Plugin` and implements `load()` and `unload()`. The
library provides a static `PluginFactory` and exports its address through the
required getter:

```cpp
#include <tobasa/exp_imp.h>
#include <tobasa/plugin.h>

namespace tbs {

class MetricsPlugin final : public Plugin
{
public:
   void load() override {}
   void unload() override {}
};

static PluginFactory<MetricsPlugin> metricsFactory(
   "Metrics Plugin",
   "metrics_plugin",
   "Example Team",
   "1.0.0",
   "Collects application metrics",
   PluginType::addon);

TBS_PLUGIN_API IPluginFactory* GetTobasaPlugin()
{
   return &metricsFactory;
}

} // namespace tbs
```

`TBS_PLUGIN_API` exports the function from the shared library. The symbol name
must be exactly `GetTobasaPlugin`; the loader does not search for alternate
entry points. The factory's name, ID, author, version, description, and type
are metadata. Tobasa does not use them to perform API-version compatibility
checks.

## Load and Use

The host loads a plugin by passing the shared-library path to the singleton
manager. It can retrieve a plugin by its factory ID:

```cpp
#include <stdexcept>
#include <string>
#include <tobasa/plugin_manager.h>

void loadMetricsPlugin(const std::string& pluginPath)
{
   auto& manager = tbs::PluginManager::get();
   if (!manager.loadPlugin(pluginPath))
      throw std::runtime_error("Failed to load metrics plugin");

   tbs::Plugin* plugin = manager.getPluginById("metrics_plugin");
   if (!plugin)
   {
      manager.unloadAllPlugins();
      throw std::runtime_error("Metrics plugin factory ID was not found");
   }

   manager.unloadAllPlugins();
}
```

`loadPlugin()` loads the shared library, resolves the exported factory,
creates the plugin object, then calls `Plugin::load()`. It returns `false` if
the library, export, factory, or plugin initialization fails. A host can call
`loadLibrary()` instead when it wants to load and register the library without
creating a plugin instance.

`PluginManager` owns each `PluginLibrary`, which owns the dynamic-library
handle and coordinates the factory and plugin instance. On unload, Tobasa
calls `Plugin::unload()`, destroys the instance through its factory, and then
closes the shared library. `unloadAllPlugins()` performs this sequence for all
registered libraries; the manager also does so during destruction.

The manager's lookup names are based on the library filename stem, not the
factory's display name. For example, `plugin_hello.dll` is registered as
`plugin_hello`; `getPlugin("plugin_hello")`, `hasPlugin()`, and
`unloadPlugin("plugin_hello")` use that name. Use `getPluginById()` for the
factory ID. `getPluginNames()` returns library filename stems.

Plugin pointers returned by the manager are non-owning. Do not retain or use
one after unloading its library. The manager locks its own registry operations,
but that lock does not protect a pointer while plugin code is being used by
the caller.

`loadPluginsFromFolder()` scans each existing folder and attempts to load every
regular file as a plugin library. Pass directories containing only intended
plugins, and only load plugin libraries from trusted sources: plugins execute
inside the host process with its permissions. `setPluginAutoLoad()` and
`isPluginAutoLoad()` store an in-memory preference; folder scanning does not
currently consult that preference.

## Platform Loading

`DynamicLib` uses `LoadLibraryA` and `GetProcAddress` on Windows. On POSIX
platforms it uses `dlopen` with `RTLD_LAZY` and `dlsym`. Build plugins as
shared libraries for the host platform (`.dll`, `.so`, or `.dylib`) and make
their dependent libraries available to the platform loader. The sample host
and plugins are under [`src/samples/using_plugin`](../src/samples/using_plugin/).

## App Server Plugins

The app server checks the directory in `TOBASA_PLUGIN_DIR`, when set, and the
`plugins` directory beside the executable. It passes those directories to
`loadPluginsFromFolder()` during startup. In shared-library builds, CMake also
builds `session_store_sqlite_plugin` and copies it into the executable's
`plugins` directory.

The web layer provides a specialized plugin interface,
`tbs::web::SessionStorePlugin`. It adds
`createStore(const SessionStoreConfig&)`, which returns an owning
`std::unique_ptr<ISessionStore>`, to the normal plugin lifecycle. The app
server's session configuration selects
the backend with `webapp.webService.sessionStore`. The built-in SQLite plugin
ID is `session_store_sqlite_plugin`; `sqlite` and `sqlite3` are accepted
aliases. The default `filesystem` backend is built into Tobasa Web and does
not require a plugin. The SQLite plugin creates its database at
`<appDataDir>/session_store.db`.

To add another session backend, implement `SessionStorePlugin`, return an
`ISessionStore` from `createStore()`, export the same factory entry point, and
place the built library in a directory the app server scans. See
[`session_store_plugin.h`](../src/tobasaweb/include/tobasaweb/session_store_plugin.h)
for the specialized interface.
