# Tobasa Build Modes

The build has three independent choices: whether Tobasa libraries are shared,
whether shared modules are combined into one library, and how third-party
dependencies are provided. The settings below are passed when configuring a
fresh build directory.

## Mode Matrix

| Mode | `TOBASA_BUILD_SHARED_LIB` | `TOBASA_BUILD_MONOLITHIC_SHARED_LIB` | `TOBASA_BUILD_WITH_VCPKG` | Dependency setup |
| --- | --- | --- | --- | --- |
| Shared component libraries | `ON` | `OFF` | `OFF` | Windows: configure the dependency paths in `CMakeDefaults.txt`; Linux: use installed system dependencies. |
| Shared monolithic library | `ON` | `ON` | `OFF` | Windows: configure the dependency paths in `CMakeDefaults.txt`; Linux: use installed system dependencies. |
| Shared component libraries with VCPKG | `ON` | `OFF` | `ON` | Windows/MSVC only; use the `tbs-shared` VCPKG triplet. |
| Shared monolithic library with VCPKG | `ON` | `ON` | `ON` | Windows/MSVC only; use the `tbs-shared-monolithic` VCPKG triplet. |
| Static libraries | `OFF` | `OFF` | `OFF` | Windows: configure the dependency paths in `CMakeDefaults.txt`; Linux: use installed system dependencies. |
| Static libraries with VCPKG | `OFF` | `OFF` | `ON` | Windows/MSVC only; use the `static` VCPKG triplet. |

For example, a static VCPKG build on Windows is configured with:

```powershell
cmake -S . -B build-static-vcpkg `
    -DTOBASA_BUILD_SHARED_LIB=OFF `
    -DTOBASA_BUILD_MONOLITHIC_SHARED_LIB=OFF `
    -DTOBASA_BUILD_WITH_VCPKG=ON
```

A shared monolithic build without VCPKG uses:

```powershell
cmake -S . -B build-shared-monolithic `
    -DTOBASA_BUILD_SHARED_LIB=ON `
    -DTOBASA_BUILD_MONOLITHIC_SHARED_LIB=ON `
    -DTOBASA_BUILD_WITH_VCPKG=OFF
```

Use a separate build directory when switching modes. CMake caches options and
toolchain settings in each build directory.

## Option Behavior

- `TOBASA_BUILD_SHARED_LIB` defaults to `ON`. On MSVC, it selects the dynamic
   MSVC runtime; `OFF` selects the static MSVC runtime and static Tobasa
   component libraries.
- `TOBASA_BUILD_MONOLITHIC_SHARED_LIB` defaults to `OFF` and is dependent on
   `TOBASA_BUILD_SHARED_LIB`. If shared mode is `OFF`, CMake forces monolithic
   mode `OFF`. When enabled, Tobasa modules are compiled into
   `tobasa_monolithic`.
- Monolithic mode requires CMake 3.13 or newer because it uses policy CMP0079.
- `TOBASA_BUILD_WITH_VCPKG` defaults to `ON`, but the top-level project permits
   VCPKG only with MSVC. Set it to `OFF` for Linux and other non-MSVC builds.
- On Windows, `CMakeDefaults.txt` selects the VCPKG triplet from the library
   mode: `static`, `tbs-shared`, or `tbs-shared-monolithic`. Its default
   `MSVS_VER` is `vs2022`; the compiler, triplet, overlay ports, and dependency
   packages must use matching versions and architectures.
- With VCPKG disabled on Windows, dependency directories are derived from
   `MSVS_LINK` and `MSVS_VER` in `CMakeDefaults.txt`. Set those paths to match
   the selected compiler and linkage. The current top-level configuration sets
   bundled zlib ON by default for fresh build directories on this path.
- On Linux, VCPKG and the Windows prebuilt-library paths are not used. Install
   development packages for enabled system dependencies and configure with
   `TOBASA_BUILD_WITH_VCPKG=OFF`.

## Zlib and SQLite Selection

These settings select how the build obtains zlib and SQLite. They are
independent of whether Tobasa's own libraries are static, shared, or
monolithic.

### Zlib

- `TOBASA_USE_PREBUILT_ZLIB` is intended to select the Windows prebuilt zlib
   libraries under `TOBASA_ZLIB_DIR`. The top-level `CMakeLists.txt` currently
   sets this variable to `OFF`, so it cannot be enabled with a normal
   `-D` configure argument. The zlib paths in `CMakeDefaults.txt` are still
   used by other dependency configurations.
- `TOBASA_USE_BUNDLED_ZLIB` selects the bundled zlib source under
   `src/ext/zlib`. On fresh MSVC non-VCPKG configurations, the top-level build
   creates this cache setting as `ON`. On non-Windows configurations it sets
   the variable to `OFF`; VCPKG configurations use `find_package(ZLIB)` when
   bundled zlib is not selected. The HTTP and MariaDB connector targets use
   this setting to choose between bundled zlib and a discovered ZLIB package.
- With bundled zlib disabled, the HTTP and MariaDB connector configuration
   falls back to `find_package(ZLIB)` unless the prebuilt-zlib branch is active.
   Because prebuilt zlib is currently forced off, that fallback is the active
   non-bundled path.

### SQLite

- `TOBASA_USE_BUNDLED_SQLITE3_MC` is the user-facing CMake option. It defaults
   to `ON` and selects the bundled SQLite3 Multiple Ciphers sources.
- `TOBASA_USE_SQLITE3_MC` is derived by `tobasa_configure_sqlite()`; it is not
   an independent option. It is set to `OFF` only when dependency-package
   lookup is enabled (`TOBASA_FIND_DEPS` or `TOBASA_BUILD_WITH_VCPKG`) and
   `TOBASA_USE_BUNDLED_SQLITE3_MC` is `OFF`. In that case CMake requires the
   `unofficial-sqlite3` package. Otherwise the bundled SQLite3 MC sources are
   used, including when the bundled option is `OFF` but package lookup is not
   enabled.
- To use SQLite from VCPKG instead of the bundled SQLite3 MC sources, set
   both `TOBASA_BUILD_WITH_VCPKG=ON` and
   `TOBASA_USE_BUNDLED_SQLITE3_MC=OFF`. Do not pass
   `TOBASA_USE_SQLITE3_MC` as a configure option; the SQLite setup overwrites
   it based on the conditions above.


