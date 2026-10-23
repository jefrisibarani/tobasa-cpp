# Building Tobasa C++ Framework

Tobasa uses CMake. Configure the build from the repository root, then build the generated tree. The project requires C++17; CMake enables C++20 where the compiler and standard library support the required features.

## Prerequisites

- CMake 3.12 or newer. Enabling the monolithic shared library requires CMake 3.13 or newer.
- A C++17-capable compiler. The repository has Windows/MSVC and Linux/GCC build paths.
- OpenSSL development files. HTTP/2 is enabled by default and also needs nghttp2.
- Dependencies for enabled SQL drivers: PostgreSQL, ODBC, SQLite, and MariaDB/MySQL are enabled by default. ADO is applicable to Windows. Some dependencies are bundled; others are found through VCPKG, CMake package discovery, or the paths in `CMakeDefaults.txt`.
- GoogleTest is fetched by CMake when `TOBASA_BUILD_TESTS` is enabled, so configuration needs network access unless that dependency is already available to FetchContent.

## Dependency Setup

### Windows

VCPKG is enabled by default and supported only with MSVC. Set
`VCPKG_TOOLCHAIN_FILE_DEF`, `VCPKG_OVERLAY_PORTS_DEF`, and
`VCPKG_TARGET_TRIPLET_DEF` in `CMakeDefaults.txt` for the local VCPKG
installation and selected Visual Studio version. The checked-in defaults
point to `d:/vcpkg` and select `vs2022` packages. Set
`TOBASA_BUILD_WITH_VCPKG=OFF` to use the prebuilt dependency paths instead.

The compiler, VCPKG triplet, and prebuilt libraries must use compatible
architectures, toolsets, and runtime linkage.

### Linux

Set `TOBASA_BUILD_WITH_VCPKG` to `OFF` when configuring on Linux. Install development packages for enabled system dependencies. On Ubuntu, a typical starting set is:

```bash
sudo apt-get update
sudo apt-get install build-essential cmake pkg-config libssl-dev zlib1g-dev \
   libpq-dev unixodbc-dev libnghttp2-dev
```

The MariaDB connector and SQLite encryption library can be built from bundled sources. Disable HTTP/2 with `-DTOBASA_HTTP_USE_HTTP2=OFF` if nghttp2 is not installed. Additional packages may be required for enabled features or tests.

## Configure and Build

### Windows with Visual Studio 2022

First configure the VCPKG or prebuilt dependency paths as described above. The checked-in dependency defaults select Visual Studio 2022 packages.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug --parallel
cmake --build build --config Release --parallel
```

The checked-in `build_all.cmd` uses the Visual Studio 2017 generator, while
`CMakeDefaults.txt` defaults to VS 2022 dependency packages. If using that
helper, update its generator and the dependency version settings so they
match. The helper configures one `build` directory and builds both Debug and
Release.

### Linux with GCC

```bash
cmake -S . -B build -G "Unix Makefiles" \
   -DTOBASA_BUILD_WITH_VCPKG=OFF \
   -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

For a Debug build, configure a separate directory with `-DCMAKE_BUILD_TYPE=Debug`. The `build_all.sh` helper does this in `build_d` and `build_r`.

## Main Build Options

Options and defaults are defined in the top-level `CMakeLists.txt`. Pass overrides to `cmake -S . -B build`.

| Option | Default | Effect |
| --- | --- | --- |
| `TOBASA_BUILD_SHARED_LIB` | `ON` | Build Tobasa modules as shared libraries and use the dynamic MSVC runtime. Set `OFF` for static libraries and the static MSVC runtime. |
| `TOBASA_BUILD_MONOLITHIC_SHARED_LIB` | `OFF` | Combine Tobasa modules into one shared library. Requires `TOBASA_BUILD_SHARED_LIB=ON`; CMake forces this option off when shared builds are disabled. |
| `TOBASA_BUILD_WITH_VCPKG` | `ON` | Use VCPKG. Supported only with MSVC; explicitly set `OFF` on Linux. |
| `TOBASA_BUILD_LIS_ENGINE` | `ON` | Include the LIS engine. |
| `TOBASA_BUILD_TESTS` | `ON` | Build test modules. GoogleTest is fetched during configuration. |
| `TOBASA_BUILD_TESTS_SQLDB_INTEGRATION` | `ON` | Include SQL database integration tests when tests and the relevant driver are enabled. |
| `TOBASA_BUILD_TESTS_WSCHAT_USE_PROBUF` | `ON` | Include protobuf-based websocket tests; this option is disabled when tests are disabled. |
| `TOBASA_HTTP_USE_HTTP2` | `ON` | Enable HTTP/2 support; nghttp2 is required. |
| `TOBASA_BUILD_IN_MEMORY_RESOURCES` | `ON` | Compile selected application resources into the executable. |
| `TOBASA_BUILD_IN_MEMORY_TZDB` | `ON` | Compile timezone data into the build when the platform does not provide the required standard library support. |

The SQL driver options `TOBASA_SQL_USE_PGSQL`, `TOBASA_SQL_USE_ADODB`, `TOBASA_SQL_USE_ODBC`, `TOBASA_SQL_USE_SQLITE`, and `TOBASA_SQL_USE_MYSQL` default to `ON`. The current SQL CMake configuration forces SQLite support on, so `TOBASA_SQL_USE_SQLITE=OFF` does not disable it.

Examples:

```bash
# Static libraries, no tests, and no HTTP/2
cmake -S . -B build-static \
   -DTOBASA_BUILD_WITH_VCPKG=OFF \
   -DTOBASA_BUILD_SHARED_LIB=OFF \
   -DTOBASA_BUILD_TESTS=OFF \
   -DTOBASA_HTTP_USE_HTTP2=OFF

# One shared Tobasa library (CMake 3.13 or newer)
cmake -S . -B build-monolithic \
   -DTOBASA_BUILD_SHARED_LIB=ON \
   -DTOBASA_BUILD_MONOLITHIC_SHARED_LIB=ON
```

## Tests and Targets

Run registered tests after building:

```bash
ctest --test-dir build --output-on-failure
```

For a multi-configuration generator such as Visual Studio, select the configuration with `-C Debug` or `-C Release`. SQL integration tests may require accessible database services and test configuration. To see the targets generated for the current options and generator, run `cmake --build build --target help`.

## Outputs

The default package destination is `_output` and can be changed with `TOBASA_PACKAGE_OUTPUT_DIR`. Application and sample targets with packaging commands copy their runtime files into a per-target directory under `_output`, with `debug` or `release` configuration subdirectories. Test executables are placed under the build tree; exact targets and outputs depend on enabled options and the selected generator.

## Troubleshooting

- **VCPKG is rejected on Linux:** configure with `-DTOBASA_BUILD_WITH_VCPKG=OFF`.
- **CMake cannot find a dependency:** check the enabled feature options, installed development packages, and the corresponding paths in `CMakeDefaults.txt`. With VCPKG, verify the toolchain, overlay ports, and triplet match the compiler.
- **Monolithic mode is unavailable:** it requires shared-library mode and CMake 3.13 or newer. Configure with `TOBASA_BUILD_SHARED_LIB=ON` and `TOBASA_BUILD_MONOLITHIC_SHARED_LIB=ON`.
- **Tests fail to configure or build:** tests are enabled by default and CMake fetches GoogleTest. Check network access and the configure output.
- **Linker errors:** confirm that third-party libraries match the target architecture, compiler toolset, and runtime linkage.

For custom Windows dependency locations, edit `CMakeDefaults.txt`. For IDE builds, open the generated solution in Visual Studio or configure the repository with the VS Code CMake Tools extension.