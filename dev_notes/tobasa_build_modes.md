# Tobasa Build Modes

The build has three independent choices: whether Tobasa libraries are shared,
whether shared modules are combined into one library, and how third-party
dependencies are provided. The settings below are passed when configuring a
fresh build directory.

## Mode Matrix

| Mode | `TOBASA_BUILD_SHARED_LIB` | `TOBASA_BUILD_MONOLITHIC_SHARED_LIB` | `TOBASA_BUILD_WITH_VCPKG` | Dependency setup |
| --- | --- | --- | --- | --- |
| Shared component libraries | `ON` | `OFF` | `OFF` | Windows: configure dependency paths in `CMakeDefaults.txt` where prebuilt branches are used; Linux: bundled or fetched sources provide zlib, SQLite3 MC, MariaDB Connector/C, and nghttp2, with system packages for discovered dependencies. |
| Shared monolithic library | `ON` | `ON` | `OFF` | Windows: configure dependency paths in `CMakeDefaults.txt` where prebuilt branches are used; Linux: bundled or fetched sources provide zlib, SQLite3 MC, MariaDB Connector/C, and nghttp2, with system packages for discovered dependencies. |
| Shared component libraries with VCPKG | `ON` | `OFF` | `ON` | Windows/MSVC only; use the generated `x64-windows-tbs-shared-vs2022`-style triplet. |
| Shared monolithic library with VCPKG | `ON` | `ON` | `ON` | Windows/MSVC only; use the generated `x64-windows-tbs-shared-monolithic-vs2022`-style triplet. |
| Static libraries | `OFF` | `OFF` | `OFF` | Windows: configure dependency paths in `CMakeDefaults.txt` where prebuilt branches are used; Linux: bundled or fetched sources provide zlib, SQLite3 MC, MariaDB Connector/C, and nghttp2, with system packages for discovered dependencies. |
| Static libraries with VCPKG | `OFF` | `OFF` | `ON` | Windows/MSVC only; use the generated `x64-windows-static-vs2022`-style triplet. |

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

- `TOBASA_BUILD_SHARED_LIB` defaults to `OFF`. On MSVC, it selects the dynamic
   MSVC runtime; `OFF` selects the static MSVC runtime and static Tobasa
   component libraries.
- `TOBASA_BUILD_MONOLITHIC_SHARED_LIB` defaults to `OFF` and is dependent on
   `TOBASA_BUILD_SHARED_LIB`. If shared mode is `OFF`, CMake forces monolithic
   mode `OFF`. When enabled, Tobasa modules are compiled into
   `tobasa_monolithic`.
- Monolithic mode requires CMake 3.13 or newer because it uses policy CMP0079.
- `TOBASA_BUILD_WITH_VCPKG` defaults to `OFF`, but the top-level project permits
   VCPKG only with MSVC. Set it to `OFF` for Linux and other non-MSVC builds.
- On Windows, `CMakeDefaults.txt` selects the VCPKG triplet from the library
   mode: `static`, `tbs-shared`, or `tbs-shared-monolithic`. Its default
   `MSVS_VER` is `vs2022`; the compiler, triplet, overlay ports, and dependency
   packages must use matching versions and architectures.
- With VCPKG disabled on Windows, dependency directories are derived from
   `MSVS_LINK` and `MSVS_VER` in `CMakeDefaults.txt`. Set those paths to match
   the selected compiler and linkage. The current top-level configuration
   uses prebuilt libpq and builds or fetches zlib, SQLite3 MC, nghttp2, and
   MariaDB Connector/C from source by default.
- On Linux, VCPKG and the Windows prebuilt-library paths are not used. Install
   development packages for enabled system dependencies and configure with
   `TOBASA_BUILD_WITH_VCPKG=OFF`.

## Zlib and SQLite Selection

These settings select how the build obtains zlib and SQLite. They are
independent of whether Tobasa's own libraries are static, shared, or
monolithic.

### Zlib

- `TOBASA_USE_PREBUILT_ZLIB` selects the prebuilt zlib libraries under
   `TOBASA_ZLIB_DIR` when it is `ON`. The top-level cache default is `OFF`, but
   an explicit cache value can enable the branch.
- When prebuilt zlib is `OFF`, VCPKG or `TOBASA_FIND_DEPS=ON` uses
   `find_package(ZLIB)`. With both options `OFF`, CMake builds the bundled
   `src/ext/zlib` tree or fetches zlib version `v1.3.2`. The internal
   `TOBASA_USE_BUNDLED_ZLIB` variable records that source choice; it is not the
   user-facing selector.

### SQLite

- `TOBASA_USE_PREBUILT_SQLITE3=ON` selects the prebuilt SQLite library under
   `TOBASA_SQLITE3_DIR`.
- Otherwise, `TOBASA_BUILD_WITH_VCPKG=ON` uses `unofficial-sqlite3` from VCPKG.
   `TOBASA_FIND_DEPS=ON` without VCPKG uses `find_package(SQLite3 CONFIG)`.
- With both options `OFF`, CMake builds bundled `src/ext/sqlite3mc` or fetches
   SQLite3 MC version `v2.6.1`. The internal `TOBASA_USE_SQLITE3_MC` value is
   derived from the selected branch and should not be passed as a configure
   option.
- `TOBASA_SQL_USE_SQLITE=OFF` is currently forced back to `ON` by the SQL
   module, so it does not disable the SQLite driver.


