# DummyPlugin

DummyPlugin is a minimal implementation of the Tobasa plugin system, showing:
- How to create a plugin class inheriting from `tbs::Plugin`
- Implementing required `load()` and `unload()` methods
- Creating a `PluginFactory` for plugin instantiation
- Exporting the `GetTobasaPlugin()` entry point for dynamic loading

## Building

The plugin is built automatically as part of the main build:
The plugin is a separate shared-library target. Package output is placed in
`_output/plugin_dummy/debug/` for Debug builds. Its filename is
`plugin_dummy.dll` on Windows and normally `libplugin_dummy.so` on Linux.

## Usage

Load the plugin using the `PluginManager`:

```cpp
#include <stdexcept>
#include <string>
#include <tobasa/plugin_manager.h>

void loadDummyPlugin(const std::string& pluginPath)
{
   auto& manager = tbs::PluginManager::get();
   if (!manager.loadPlugin(pluginPath))
      throw std::runtime_error("Failed to load plugin");

   auto* plugin = manager.getPluginById("dummy_plugin");
   if (!plugin)
   {
      manager.unloadPlugin("plugin_dummy");
      throw std::runtime_error("Dummy plugin factory ID was not found");
   }

   // Use the plugin while its shared library remains loaded.
   manager.unloadPlugin("plugin_dummy");
}
```

`PluginManager::loadPlugin()` calls the plugin's `load()` method, and
`unloadPlugin()` calls `unload()` before destroying the instance and closing
the library. Do not call those lifecycle methods directly. The manager lookup
name `plugin_dummy` comes from the library filename stem; the factory ID is
`dummy_plugin`.

## Plugin Metadata

- **Name**: Dummy Plugin
- **ID**: dummy_plugin
- **Author**: Tobasa Team
- **Version**: 1.0.0
- **Type**: addon
- **Description**: A simple dummy plugin for testing

## Files

- `src/dummy.h` - Plugin class header
- `src/dummy.cpp` - Plugin class implementation, factory, and export function
- `CMakeLists.txt` - Build configuration

## See Also

- [plugin.h](../../tobasa/include/tobasa/plugin.h) - Plugin base class and factory interface
- [plugin_lib.h](../../tobasa/include/tobasa/plugin_lib.h) - Plugin library loader
- [plugin_manager.h](../../tobasa/include/tobasa/plugin_manager.h) - Plugin manager
