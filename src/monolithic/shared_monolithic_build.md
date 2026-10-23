# Shared Monolithic Build

The shared monolithic configuration compiles Tobasa's core, SQL, HTTP, web,
and other enabled modules (including LIS when `TOBASA_BUILD_LIS_ENGINE=ON`)
into one shared library: `tobasa_monolithic`. The
application can continue to link against the familiar component target names;
in this configuration those names forward to the monolithic library.

To select a shared monolithic build, enable both options:

```cmake
TOBASA_BUILD_SHARED_LIB=ON
TOBASA_BUILD_MONOLITHIC_SHARED_LIB=ON
```

`TOBASA_BUILD_MONOLITHIC_SHARED_LIB` is dependent on
`TOBASA_BUILD_SHARED_LIB` and defaults to `OFF`. The overall defaults are
shared component libraries (`TOBASA_BUILD_SHARED_LIB=ON`,
`TOBASA_BUILD_MONOLITHIC_SHARED_LIB=OFF`). Disable the monolithic option to
build separate shared libraries, or disable shared libraries to build static
modules; CMake forces the dependent monolithic option off in static mode.

## Configure and Assemble

The root [`CMakeLists.txt`](../../CMakeLists.txt) adds `src/monolithic` before
adding the individual module directories. This ordering creates the shared
target before each module tries to add its sources.

[`CMakeLists.txt`](CMakeLists.txt) creates the shared library target with the
platform export definitions, then lists the module names:

```cmake
add_library(tobasa_monolithic SHARED)
```

For the monolithic option, each module sets `TOBASA_MODULE_TARGET` to
`tobasa_monolithic`. Its `target_sources`, include directories, compile
definitions, and required external libraries are therefore applied to that
one target. For example, `tobasahttp` adds its sources and Asio/OpenSSL
requirements to the shared target. Internal module-to-module library links are
skipped where the modules are already part of the same target.

The resulting DLL contains the compiled code from the enabled modules. It is
not a separate wrapper DLL that dynamically loads the component DLLs.

## Component Target Names

The monolithic setup creates `INTERFACE` targets named `tobasa`, `tobasahttp`,
`tobasaweb`, `tobasasql`, and `tobasalis`. Each interface target
links to `tobasa_monolithic`. These targets preserve the standard component
names and propagate the monolithic target's usage requirements to consumers.
The `tobasa::monolithic` alias is also available for code that wants to name
the shared target directly.

This is why [`src/app_server/CMakeLists.txt`](../app_server/CMakeLists.txt)
can keep its normal component link list:

```cmake
target_link_libraries(webservice PRIVATE tobasa tobasasql tobasahttp tobasaweb)
```

In monolithic mode, those interface targets resolve to `tobasa_monolithic`.
The same application link declaration also works in the separate-library
configuration, where the component names refer to actual library targets.

## Runtime DLL Staging

The app server calls
[`tobasa_stage_shared_runtime`](../../cmake/tobasa_stage_shared_runtime.cmake)
after linking. On Windows shared builds, the helper detects monolithic mode,
copies `tobasa_monolithic` beside the executable, and adds its output
directory to the runtime dependency search paths. With CMake 3.16 or newer,
the post-build script uses `file(GET_RUNTIME_DEPENDENCIES)` to copy resolved
non-system DLL dependencies into the executable's output directory. Windows
system DLLs and API-set DLLs are excluded; unresolved or conflicting
dependencies produce warnings.

When the monolithic option is off, the helper instead stages the individual
shared component libraries. Runtime staging is skipped when shared builds are
disabled or on non-Windows platforms.

## Build

With a build tree already configured for the monolithic option, build the
library and application targets with:

```powershell
cmake --build build --target tobasa_monolithic webservice
```

The exact configure command may also need the project's toolchain and
dependency settings; see the root CMake configuration for those options.
