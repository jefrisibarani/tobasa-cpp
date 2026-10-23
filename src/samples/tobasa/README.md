# Tobasa Plugin Sample

The sample application loads the `plugin_dummy` shared library and demonstrates
the plugin manager API:
- Loading the plugin library
- Creating and initializing the plugin
- Calling plugin methods
- Unloading the plugin

## Building

The sample is built as part of the main build system:
The compiled executable will be in `_output/test_tobasa/debug/`

## Running

1. Build the `test_tobasa` and `plugin_dummy` targets.
2. Update the hard-coded plugin path in `src/main.cpp` to the built plugin file on your machine.
3. Run the sample:
   ```bash
   ./test_tobasa
   ```

`PluginManager::loadPlugin()` invokes `Plugin::load()` after creating the
plugin instance. `unloadAllPlugins()` invokes `Plugin::unload()` before
destroying the instance and closing the library. The sample leaves those
lifecycle calls to the manager and only checks that the plugin was created.

## Files

- `main.cpp`: Main application that loads and tests the dummy plugin
- `CMakeLists.txt`: Build configuration for the sample

## Dependencies

- Tobasa framework
- Dummy plugin (built separately)

## Notes

- `getPlugin("plugin_dummy")` uses the library filename stem; the plugin factory ID is `dummy_plugin`.
- On Linux/macOS, the sample source path and library filename need to be changed to match the built `.so` or `.dylib`.