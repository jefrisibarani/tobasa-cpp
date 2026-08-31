# Tobasa Web Service Application

A production-grade REST API and web service demonstrating enterprise-level architecture, scalability, and best practices using the Tobasa framework.

## Overview

The **app_server** (webservice) is a complete, deployable web service application built with the Tobasa framework. It demonstrates how to architect a modern REST API with layered controllers, pluggable middleware, JWT/session authentication, database migrations, and multi-protocol support (HTTP/1.1, HTTP/2, WebSocket, TLS). Suitable for immediate adaptation to production workloads.

## Features

### Core Capabilities

- **HTTP/HTTPS Server**: Built on `tobasahttp` with support for both HTTP/1.1 and HTTP/2 protocols
- **TLS/SSL Support**: Secure communication with configurable certificates
- **REST Routing**: Dynamic request routing and controller-based handlers via `tobasaweb`
- **Database Integration**: SQL abstraction layer supporting SQLite, MySQL, PostgreSQL, and MSSQL via `tobasasql`
- **WebSocket Support**: Real-time bidirectional communication
- **Configuration Management**: Flexible configuration system with embedded defaults
- **Request Middleware**: Pluggable middleware pipeline for request/response processing
- **Resource Embedding**: Support for embedding static assets (views, styles, scripts)

### Advanced Features

- **Session Management**: Built-in session handling and JWT support
- **LIS Protocol Support**: Optional integration with `tobasalis` for healthcare instrument communication
- **Custom Matchers**: Pattern matching for routing (regex, wildcard support)
- **Database Migrations**: Structured schema management
- **Timezone Support**: Embedded timezone data or system-based timezone handling
- **Protobuf Integration**: Optional Protocol Buffer message support

## Directory Structure

```
app_server/
├── src/                     # Application source code
│   ├── main.cpp             # Entry point
│   ├── main_helper.cpp      # Startup utilities
│   ├── app_common.cpp       # Common application logic
│   ├── app_resource.cpp     # Resource management
│   ├── app_util.cpp         # Utility functions
│   ├── core/                # Core components
│   ├── middleware/          # Request middleware implementations
│   ├── lis/                 # LIS protocol support modules
│   ├── db_migrations/       # Database schema migrations
│   ├── test/                # Tests (SQL, crypto, date time and upload)
│   └── test_ws/             # WebSocket test
├── configuration/           # Runtime configuration files
├── configuration_embed/     # Embedded configuration defaults
├── views/                   # HTML/template files
├── views_lis/               # LIS-specific view templates
├── wwwroot/                 # Static web assets (CSS, JS, images)
├── tls_asset/               # TLS certificates and keys
├── appdata/                 # Application data directory
├── cmake/                   # CMake build configuration
├── VERSION                  # Version information
└── README.md                # This file
```

## Building

### Build from Top-Level

The recommended way is to build from the project root:

```bash
# Windows
.\build_all.cmd

# Linux
./build_all.sh
```

### Build Just This Component

```bash
# Configure with CMake
cmake -B build -S .

# Build
cmake --build build --target webservice --config Debug

# Or use your build script
./build_all.cmd  # Windows
./build_all.sh   # Linux
```

The built executable will be located in `_output/webservice/debug/` or the configured output directory.
### Directory Structure
```
debug/
├── appdata/          # Application data folder
├── configuration/    # Runtime configuration
├── tls_asset/        # TLS certificates and keys
├── views/            # HTML/template files
├── wwwroot/          # Static web assets (CSS, JS, images)
├── tzdata/           # Time zone data
└── webservice.exe
```
With CMake option `TOBASA_BUILD_IN_MEMORY_RESOURCES`, `TOBASA_BUILD_IN_MEMORY_TZDB` ON
we can safely delete views and wwwroot and /tzdata


## Configuration

### Configuration Files

Configuration is handled through:

1. **Embedded Defaults** (`configuration_embed/`): Default settings compiled into the binary
2. **Runtime Configuration** (`configuration/`): Override defaults at runtime


### TLS/SSL Setup

TLS certificates and keys should be placed in the `tls_asset/` directory:

- Server certificate: typically `server.crt` or `server.pem`
- Private key: typically `server.key`
- CA certificate (optional): `ca.crt`

### Database Configuration

The application uses `tobasasql` for database access with support for:

- **SQLite**
- **MySQL/MariaDB**
- **PostgreSQL**
- **MSSQL**: Via ODBC or ADO

Configure the database connection in the configuration files.

### Web Assets

- **Views/Templates**: HTML and template files in `views/` directory
- **Static Files**: CSS, JavaScript, images in `wwwroot/` directory
- **LIS Views**: Specialized views for LIS integration in `views_lis/`

The application can embed these resources into the binary at build time (set `TOBASA_BUILD_IN_MEMORY_RESOURCES` CMake option).

## Running the Application

After building, run the executable:

```bash
./_output/webservice/debug/webservice
```

Or from the build directory:

```bash
./build/Debug/webservice  # MSVC
./build/webservice        # GCC
```

## Key Components

### Application Initialization (`main.cpp`, `main_helper.cpp`)

Startup sequence for a production-ready application:

1. **Timezone Initialization** – Loads timezone database (in-memory or system)
2. **Configuration Loading** – Merges embedded defaults with runtime config files (`appsettings.json`)
3. **Logging Setup** – Initializes structured logging via `tobasa` with configurable levels
4. **Database Service** – Creates connection pool and registers with `tobasasql` abstraction layer
5. **Database Migrations** – Automatically applies pending schema migrations on startup
6. **Middleware Pipeline** – Registers 10-layer middleware chain in execution order
7. **Controller Registration** – Registers route handlers (CoreController, ApiCoreController, ApiUsersController, AdminController)
8. **TLS Asset Callbacks** – Sets up resource loading for certificates and keys (embedded or filesystem)
9. **Lifecycle Hooks** – Registers `onStart()` and `onStop()` handlers (e.g., for LIS engine startup)
10. **Thread Pool Setup** – Configures IO thread pool and worker thread pool for request handling
11. **Server Start** – Launches HTTP/HTTPS listeners and enters async event loop

### Controllers & Routing (`core/`)

Request handlers organized by domain concern, following the MVC/MVT pattern. Routes are matched using configurable matchers (regex or wildcard patterns) with per-route authentication/authorization rules.

**Controllers:**

- **CoreController** – Primary UI handlers for authentication, dashboards, user profiles, login/logout, resource serving
  - Routes: `/`, `/index`, `/dashboard`, `/login`, `/register`, `/logout`, `/password`, `/user_profile/{id}`, `/resource/{type}/{name}`

- **ApiCoreController** – Core API endpoints for versioning, server status, encryption/decryption, logs, configuration
  - Routes: `/api/version`, `/api/server_status`, `/api/authenticate`, `/api/refresh_auth_token`, `/api/decrypt`, `/api/encrypt`, `/api/running_configuration`

- **ApiUsersController** – User management REST API
  - Routes: `/api/users`, `/api/users/{id}`, `/api/users/{id}/roles`, etc.

- **AdminController** – Administrative dashboard and system management
  - Routes: `/admin`, `/admin/dashboard`, `/admin/system`, etc.

**Route Features:**

- **Pattern Matching** – Supports path parameters: `/api/users/{id}/roles/{roleId}`
- **Authentication** – Per-route JWT or session-based auth via `RouteAuth` configuration
- **Authorization** – Role-based access control (ACL) per endpoint
- **HTTP Methods** – GET, POST, PUT, DELETE, HEAD, OPTIONS
- **Response Types** – JSON API responses, HTML pages, file downloads, redirects

Routes are configured in `bindHandler()` method of each controller and registered during application startup.

### Middleware Stack (`middleware/`)

Ordered request/response processing pipeline for cross-cutting concerns. Middleware executes sequentially before reaching the router/controllers:

1. **Exception Handler** – Catches and formats unhandled exceptions
2. **Database Check** – Validates database connectivity before processing requests
3. **Multipart Body Parser** – Parses multipart/form-data for file uploads
4. **Response Header Rules** – Applies CORS policies and custom response headers
5. **Request Identification** – Tracks request ID, client info, and User-Agent validation
6. **Cache Control** – Manages HTTP caching headers (ETag, Cache-Control, etc.)
7. **Content-Type Validation** – Validates request Content-Type before processing
8. **Session Management** – Loads/maintains user sessions with configurable expiration
9. **Authentication** – Validates JWT tokens or session cookies
10. **Authorization** – Enforces route-level access control and permissions

Custom middleware can be inserted at any point in the chain.

### Database Repository (`db_repo_app.h`)

Data access layer (DAL) abstracting database operations via `tobasasql`. Provides type-safe, multi-database compatible queries without writing SQL directly.

**Architecture:**

- **Base Repositories** – Inherit from `RepositoryBase<Entity>` providing CRUD operations (Create, Read, Update, Delete)
- **Custom Queries** – Repository methods wrap parameterized SQL calls, returning strongly-typed entities or collections
- **Entity/DTO Mapping** – Automatic JSON serialization via NLOHMANN_JSON macros (see `test_sql_json_dto.h`)
- **Connection Abstraction** – Queries run transparently against SQLite, MySQL, PostgreSQL, or MSSQL

**Example Repository Pattern:**

```cpp
class UserRepository : public RepositoryBase<User> {
public:
    std::vector<User> getAllUsers();
    User getUserById(int id);
    bool updateUser(const User& user);
};
```

**Key Features:**

- **Parameterized Queries** – Prevents SQL injection via bound parameters
- **Connection Pooling** – Reuses database connections for performance
- **Transaction Support** – Explicit transaction management for multi-step operations
- **Error Handling** – Exceptions on query failures with detailed diagnostics

Controllers instantiate repositories and call methods to fetch/persist data. No SQL strings exposed to business logic layer.

### Resource Management (`app_resource.cpp`)

Flexible asset loading system supporting both embedded binary resources and filesystem fallback. Enables containerized deployments without external file dependencies.

**Resource Types:**

- **HTML/Template Files** (`views/`, `views_lis/`) – Inja templates for rendering responses
- **Static Assets** (`wwwroot/`) – CSS, JavaScript, images served via `/resource` routes
- **Configuration Files** – `appsettings.json` with embedded defaults as fallback
- **TLS Assets** – Server certificates, private keys, DH parameters (loaded on startup)
- **Timezone Database** – In-memory timezone data (when `TOBASA_BUILD_IN_MEMORY_TZDB` enabled)

**Loading Strategy:**

1. **Embedded First** – If CMake flag `TOBASA_BUILD_IN_MEMORY_RESOURCES` is ON, resources are compiled into binary
2. **Filesystem Fallback** – If embedded resource not found, attempts to load from disk
3. **Error Handling** – Returns embedded defaults if filesystem access fails

**Configuration:**

```cpp
// In main.cpp, resource loading via callbacks:
webapp.defaultTlsAssetCallback([](http::TlsAsset asset) {
    if (asset == http::TlsAsset::cerificate_chain)
        return app::Resource::get("tls_asset/127.0.0.1.crt", "tls_asset");
    // ... fallback handling
});
```

**CMake Build Options:**

- `TOBASA_BUILD_IN_MEMORY_RESOURCES=ON` – Embeds all assets, enables single-file deployment
- `TOBASA_BUILD_IN_MEMORY_TZDB=ON` – Embeds timezone data, eliminates `tzdata/` folder requirement

With embedded resources, only the executable is needed for deployment (no external files required).

### Utilities (`app_util.cpp`, `app_common.cpp`)

Common helper functions for date/time handling, string processing, JSON manipulation, and other frequently-used operations.

## Development & Testing

### Integrated Test Modules

Tests are compiled conditionally via the `TOBASA_USE_TESTS_MODULE` CMake flag and accessible through controller endpoints.

#### SQL Database Tests (`src/test/`)

Multi-driver database compatibility tests validating `tobasasql` abstraction across all supported databases:

- **SQLite** (`test_sql_sqlite_defs.h`) – File-based database tests
- **MySQL/MariaDB** (`test_sql_mysql_defs.h`, `test_sql_odbc_mysql_defs.h`) – Native and ODBC connectors
- **PostgreSQL** (`test_sql_pgsql_defs.h`) – Native connector
- **MSSQL** (`test_sql_ado_defs.h`, `test_sql_odbc_mssql_defs.h`) – ADO.NET and ODBC connectors
- **DTO/JSON Serialization** (`test_sql_json_dto.h`) – Entity-to-JSON mapping validation

Accessible via `/test/sql` endpoint when enabled.

#### WebSocket Tests / Real-Time Chat (`src/test_ws/`)

Interactive WebSocket test application demonstrating:

- Real-time bidirectional messaging
- Connection lifecycle management (open, ping, pong, close)
- Broadcast messaging to multiple clients
- Message routing between client connections

Accessible via `/test_websocket` endpoint when enabled. Full HTML chat UI included.

#### Upload & File Handling (`src/test/`)

Tests for multipart/form-data parsing and file upload processing, validating:

- Large file uploads
- Multiple file handling
- Temporary file cleanup

### Database Migrations

Database schema changes are managed in `src/db_migrations/`. Migrations are versioned and applied automatically on startup:

- **Base Module** – Core schema (users, roles, permissions)
- **Test Module** (optional) – Test data and schemas (enabled via `TOBASA_USE_TESTS_MODULE`)
- **LIS Module** (optional) – Healthcare LIS integration schema (enabled via `TOBASA_USE_LIS_ENGINE`)

New migrations can be added as new C++ classes inheriting from the migration base.

## Dependencies

This application depends on:

- **tobasa**: Core utilities
- **tobasahttp**: HTTP/HTTPS server and client
- **tobasaweb**: Web framework and routing
- **tobasasql**: Database abstraction
- **tobasalis** (optional): LIS protocol support
- **Standard C++ Libraries**: C++17 or later

## Performance Considerations

- **Connection Pooling**: Database connection pools are configured for optimal throughput
- **Asynchronous I/O**: `tobasahttp` uses Asio for efficient async networking
- **Resource Embedding**: In-memory resources reduce filesystem I/O
- **Request Buffering**: Configurable for optimal memory usage

## Debugging

Build with debug symbols for full IDE support:

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

## License

See [LICENSE](LICENSE) in this directory for licensing information.

## Further Documentation

- Main project README: See `README.md` at repository root
- Tobasa Libraries Documentation: Each library has its own documentation
- CMake Configuration: See `cmake/` directory
- Version History: See `VERSION` file

## Support & Contributing

For issues, questions, or contributions, please refer to the main project repository.
