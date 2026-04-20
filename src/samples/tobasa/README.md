# Tobasa Plugin Sample

This sample demonstrates how to load and use plugins in the Tobasa framework.

## Overview

The sample application loads a dummy plugin (`plugin_dummy.dll`) and demonstrates the plugin lifecycle:
- Loading the plugin library
- Creating and initializing the plugin
- Calling plugin methods
- Unloading the plugin

## Building

1. Ensure the Tobasa framework is built and installed.
2. Build the dummy plugin:
   ```bash
   cd src/plugins/dummy
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   cmake --build .
   ```
3. Build the sample:
   ```bash
   cd src/samples/tobasa
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   cmake --build .
   ```

## Running

1. Ensure the plugin DLL is in the same directory as the executable or in the system PATH.
2. Run the sample:
   ```bash
   ./test_tobasa
   ```

Expected output:
```
DummyPlugin: load() called
DummyPlugin loaded successfully!
DummyPlugin: unload() called
DummyPlugin unloaded successfully!
```

## Files

- `main.cpp`: Main application that loads and tests the dummy plugin
- `CMakeLists.txt`: Build configuration for the sample

## Dependencies

- Tobasa framework
- Dummy plugin (built separately)

## Notes

- On Windows, ensure `plugin_dummy.dll` is in the executable's directory or PATH.
- On Linux/macOS, ensure `libplugin_dummy.so` is in LD_LIBRARY_PATH or the executable's directory.