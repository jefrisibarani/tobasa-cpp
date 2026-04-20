# DummyPlugin

A simple example plugin demonstrating the Tobasa plugin architecture.

## Overview

DummyPlugin is a minimal implementation of the Tobasa plugin system, showing:
- How to create a plugin class inheriting from `tbs::Plugin`
- Implementing required `load()` and `unload()` methods
- Creating a `PluginFactory` for plugin instantiation
- Exporting the `GetTobasaPlugin()` entry point for dynamic loading

## Building

The plugin is built automatically as part of the main build:

```bash
cmake --build . --target plugin_dummy
```

Or as part of the full build:

```bash
cmake --build .
```

The compiled plugin (`.dll` on Windows, `.so` on Linux) will be located in the build output directory.

## Usage

Load the plugin using the `PluginManager`:

```cpp
#include "tobasa/plugin_manager.h"

int main() 
{
   tbs::PluginManager& manager = tbs::PluginManager::get();
   
   // Load the plugin from the built library path
   if (manager.loadPlugin("path/to/plugin_dummy.dll")) 
   {
      tbs::Plugin* plugin = manager.getPlugin("plugin_dummy");
      if (plugin) 
      {
         // Plugin is now loaded
         plugin->load();  // Explicit load call (optional)
         // ... use plugin ...
         plugin->unload();  // Unload when done
      }
   }
   
   manager.unloadAllPlugins();
   return 0;
}
```

## Plugin Metadata

- **Name**: Dummy Plugin
- **ID**: plugin_dummy
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
