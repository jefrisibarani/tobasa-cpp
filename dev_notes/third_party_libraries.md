# Third-Party Libraries

Tobasa uses a mix of vendored source code, header-only dependencies, system
packages, and Windows prebuilt libraries. The selected provider depends on
platform, compiler, build options, and whether VCPKG or `TOBASA_FIND_DEPS` is
enabled. This page describes dependencies referenced by the current CMake
targets; files and paths listed in `CMakeDefaults.txt` are not necessarily
used unless a corresponding feature is enabled.

## Dependency Providers

| Library | What Tobasa uses it for | How CMake obtains and links it |
| --- | --- | --- |
| Asio | Networking in HTTP and LIS | Vendored headers at `src/ext/asio`; added as public include directories. |
| nlohmann/json | JSON support in core and web | Vendored headers at `src/ext/nlohmann`; added as a public include directory. |
| utf8cpp | UTF-8 handling in core | Vendored headers at `src/ext/utf8cpp`; added as a public include directory. |
| jwt-cpp | Web authentication/token support | Vendored headers at `src/ext/jwt-cpp`; added as a public include directory. |
| fmt | Formatting when standard formatting is unavailable | Uses the standard library when supported; otherwise uses bundled `src/ext/fmt`, or `find_package(fmt CONFIG)` with VCPKG/`TOBASA_FIND_DEPS`. The target links `fmt::fmt`. |
| date/tz | Calendar and timezone support when the standard library lacks it | Uses standard chrono when supported. Otherwise uses bundled `src/ext/date`, or `find_package(date CONFIG)` when package lookup is enabled and `TOBASA_BUILD_IN_MEMORY_TZDB=OFF`. The bundled path builds a `timezone` static target. |
| spdlog | Logging in Tobasa Web | Uses vendored headers at `src/ext/spdlog` when VCPKG and `TOBASA_FIND_DEPS` are both disabled; otherwise uses `find_package(spdlog)` and links `spdlog::spdlog`. |
| zlib | Compression in HTTP and MariaDB Connector C | Windows non-VCPKG builds use bundled `src/ext/zlib` by default. VCPKG uses `find_package(ZLIB)`; other non-bundled paths also discover ZLIB. A prebuilt Windows branch exists in module CMake, but top-level CMake currently forces `TOBASA_USE_PREBUILT_ZLIB=OFF`. |
| SQLite3 MC | SQL SQLite driver | Bundled source at `src/ext/sqlite3_mc` is selected by default. If package lookup is enabled and `TOBASA_USE_BUNDLED_SQLITE3_MC=OFF`, CMake requires `unofficial-sqlite3` and links `unofficial::sqlite3::sqlite3`. |
| MariaDB Connector C | MySQL/MariaDB SQL driver | Built from `src/ext/mariadb-connector-c` when package lookup is disabled; otherwise found as `unofficial-libmariadb`. The bundled path links `mariadbclient` by default or `libmariadb` when `TOBASA_SQL_USE_MARIADB_DLL=ON`. |
| GoogleTest | Unit and integration tests | Fetched by CMake FetchContent when tests are enabled and `GTest::gtest_main` is not already available. The default version is `v1.14.0` and can be changed with `TOBASA_GOOGLETEST_VERSION`. Configuration may need network access. |

The `src/ext` directory also contains other libraries and source trees. Their
presence there does not mean every build compiles or links them. For example,
`date`, `fmt`, zlib, SQLite3 MC, and MariaDB Connector C are conditionally
built, while the image-library directories in `CMakeDefaults.txt` are not
currently referenced by the active project CMake targets.

## Discovered and Prebuilt Dependencies

| Dependency | When it is used | Discovery/link path |
| --- | --- | --- |
| OpenSSL | Required by Tobasa HTTP | Always uses `find_package(OpenSSL REQUIRED)`. On MSVC, `TOBASA_OPENSSL_DIR` is exported as `OPENSSL_ROOT_DIR` to direct discovery to the configured prebuilt package. Linked to `tobasahttp`. |
| nghttp2 | HTTP/2, enabled by default | VCPKG builds use the selected triplet's `include`, `lib`, and `debug/lib` paths. Unix builds or builds with `TOBASA_FIND_DEPS=ON` use pkg-config (`libnghttp2`). Windows without either uses `TOBASA_HTTP2_DIR`. Disable with `TOBASA_HTTP_USE_HTTP2=OFF`. |
| PostgreSQL/libpq | PostgreSQL SQL driver, enabled by default | MSVC without VCPKG uses libraries and headers under `TOBASA_LIBPQ_DIR` (and OpenSSL paths); otherwise CMake uses `find_package(PostgreSQL REQUIRED)` and `PostgreSQL::PostgreSQL`. Disable with `TOBASA_SQL_USE_PGSQL=OFF`. |
| ODBC | ODBC SQL driver, enabled by default | Uses `find_package(ODBC REQUIRED)` and links the discovered ODBC libraries. Disable with `TOBASA_SQL_USE_ODBC=OFF`. |
| ADO | ADO SQL driver on Windows | Enabled by default on MSVC and uses Windows ADO/COM facilities; it does not use a separate VCPKG package. |
| Protocol Buffers | Websocket protobuf tests or chat module | Used when `TOBASA_BUILD_TESTS_WSCHAT_USE_PROBUF` or `TOBASA_BUILD_CHAT_MODULE` is enabled. VCPKG/`TOBASA_FIND_DEPS` uses `find_package(protobuf)`; otherwise the app server uses `TOBASA_PROTOBUF_DIR`. |

The SQL drivers above are enabled by default. SQLite is currently forced ON
inside `src/tobasasql/CMakeLists.txt`, so setting `TOBASA_SQL_USE_SQLITE=OFF`
does not remove the SQLite driver. Database services and some tests still
require the corresponding server, driver package, or local configuration.

## Effect of Tobasa Build Modes

The options in [tobasa_build_modes.md](tobasa_build_modes.md) select how
Tobasa's own module targets are produced. They do not automatically merge all
third-party code into Tobasa:

- **Static mode** builds Tobasa component libraries as static libraries and
	selects the static MSVC runtime. Third-party libraries are linked according
	to their own CMake target or configured package. Static link requirements
	propagate through the relevant Tobasa targets.
- **Shared component mode** builds separate Tobasa DLLs/shared libraries and
	selects the dynamic MSVC runtime. Dependencies remain independently linked
	libraries or packages. The Windows VCPKG triplet is `tbs-shared`.
- **Shared monolithic mode** combines Tobasa module sources in
	`tobasa_monolithic`. It still links external third-party targets/libraries;
	the monolithic option is not a promise that all dependencies are embedded
	into that one library. The Windows VCPKG triplet is
	`tbs-shared-monolithic`.
- **Windows non-VCPKG mode** derives prebuilt dependency paths from
	`MSVS_LINK` and `MSVS_VER` in `CMakeDefaults.txt`. The configured packages
	must match the compiler version, architecture, and runtime. Both shared
	triplets are documented as using static third-party libraries; Tobasa's
	runtime linkage is selected separately.
- The MySQL/MariaDB connector is a mode-specific exception: top-level CMake
	forces `TOBASA_SQL_USE_MARIADB_DLL=ON` for separate shared builds and `OFF`
	for monolithic shared builds. Static builds default it to `OFF`.
- SQLite3 MC is built as a shared target for shared component builds, and as a
	static target for static or monolithic builds. When the bundled source is
	not used, the selected package's linkage is controlled by its package
	manager/build configuration.

VCPKG defaults to `ON` and is supported only with MSVC. Set
`TOBASA_BUILD_WITH_VCPKG=OFF` for Linux and other non-MSVC configurations.
The modules also expose `TOBASA_FIND_DEPS` for package discovery without
VCPKG; behavior is still dependency-specific, so check the tables above.
