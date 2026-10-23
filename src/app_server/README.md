# Tobasa Web Service Application

`app_server` (target `webservice`) is a sample application built with the Tobasa
libraries. It demonstrates controllers, middleware, JWT and session
authentication, database migrations, HTTP/HTTPS, WebSocket, and optional LIS
integration. The checked-in configuration and TLS credentials are for
development; review and replace them before deployment.

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

- **Session Management**: Filesystem sessions by default, with the SQLite session-store plugin available in shared-library builds, plus JWT and CSRF support
- **LIS Protocol Support**: Optional integration with `tobasalis` for healthcare instrument communication
- **Route Matching**: Whole-segment path parameters and a literal `starts_with` rule
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
├── doc                      # Documentation files
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
Embedding templates and `wwwroot` requires
`TOBASA_BUILD_IN_MEMORY_RESOURCES=ON`. Timezone assets are controlled by
`TOBASA_BUILD_IN_MEMORY_TZDB`; they are not an `app::Resource` context.
Runtime configuration, application data, uploads, and database files remain
external. Disk fallback for missing embedded templates or static files is
handled by their callers, not by `app::Resource::get()`.


## Configuration

### Configuration Files

Configuration is handled through:

1. **Embedded Defaults** ([`configuration_embed/`](configuration_embed/appsettings.json)): Default settings compiled into the binary
2. **Runtime Configuration** ([`configuration/`](configuration/appsettings.json)): Override defaults at runtime


### TLS/SSL Setup

The sample uses development assets under [`tls_asset/`](tls_asset) through its
TLS asset callback. Configure certificate and key paths in `appsettings.json`
for your environment; do not deploy the checked-in development credentials.

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

### Application Initialization ([`main.cpp`](src/main.cpp), `main_helper.cpp`)

Startup sequence for the sample application:

1. **Timezone Initialization** – Loads timezone database (in-memory or system)
2. **Configuration Loading** – Merges embedded defaults with runtime config files (`appsettings.json`)
3. **Logging Setup** – Initializes structured logging via `tobasa` with configurable levels
4. **Database Service** – Creates connection pool and registers with `tobasasql` abstraction layer
5. **Database Migrations** – Automatically applies pending schema migrations on startup
6. **Middleware Pipeline** – Registers application middleware, including session, CSRF, authentication, and authorization
7. **Controller Registration** – Registers route handlers (CoreController, ApiCoreController, ApiUsersController, AdminController)
8. **TLS Asset Callbacks** – Sets up resource loading for certificates and keys (embedded or filesystem)
9. **Lifecycle Hooks** – Registers `onStart()` and `onStop()` handlers (e.g., for LIS engine startup)
10. **Thread Pool Setup** – Configures IO thread pool and worker thread pool for request handling
11. **Server Start** – Launches HTTP/HTTPS listeners and enters async event loop

### Controllers & Routing (`core/`)

Request handlers are organized by domain concern. Routes use method/path
matching with whole-segment parameters and per-route authentication and
authorization rules.

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
- **HTTP Methods** – Application routes register GET, POST, PUT, and DELETE. HTTP parser support for HEAD/OPTIONS does not add corresponding app routes.
- **Response Types** – JSON API responses, HTML pages, file downloads, redirects

Routes are configured in `bindHandler()` method of each controller and registered during application startup.

### Middleware Stack (`middleware/`)

The application registers middleware in this order before the router:

1. **Exception Handler** – Catches and formats unhandled exceptions
2. **Multipart** – Added only when internal HTTP-server multipart parsing is disabled; otherwise the HTTP server parses the body.
3. **Database Check** – Validates database connectivity before processing requests
4. **Response Header Rules** – Applies configured response headers
5. **Request Identification** – Adds request-identification data
6. **Cache Control** – Applies configured cache rules
7. **Content-Type Validation** – Validates request Content-Type
8. **Session Management** – Loads or creates a session when the route policy requires it
9. **CSRF** – Validates tokens on selected browser routes
10. **Authentication** – Validates the configured credentials
11. **Authorization** – Enforces route-level access rules

Custom middleware can be inserted at any point in the chain.

### Database Repository (`db_repo_app.h`)

Data access layer (DAL) built on `tobasasql`. Repository methods own SQL and
database-specific mapping; this is not a generated CRUD layer.

**Architecture:**

- **Repositories** – Encapsulate application queries and database operations
- **Custom Queries** – Methods use parameterized SQL and return application entities or collections
- **Entity/DTO Mapping** – Automatic JSON serialization via NLOHMANN_JSON macros (see `test_sql_json_dto.h`)
- **Connection Abstraction** – Queries run transparently against SQLite, MySQL, PostgreSQL, or MSSQL

**Key Features:**

- **Parameterized Queries** – Prevents SQL injection via bound parameters
- **Connection Pooling** – Reuses database connections for performance
- **Transaction Support** – Explicit transaction management for multi-step operations
- **Error Handling** – Exceptions on query failures with detailed diagnostics

Controllers call repository/service methods; SQL remains in the repository or
service implementation.

### Resource Management (`app_resource.cpp`)

The app server embeds selected resources and performs caller-specific
filesystem lookup where applicable. Embedding does not remove runtime
configuration, TLS, data, upload, or database-file requirements.

**Resource Types:**

- **HTML/Template Files** (`views/`, `views_lis/`) – Inja templates for rendering responses
- **Static Assets** (`wwwroot/`) – CSS, JavaScript, images served via `/resource` routes
- **Configuration Files** – `appsettings.json` with embedded defaults as fallback
- **TLS Assets** – Server certificates, private keys, DH parameters (loaded on startup)
- **Timezone Database** – In-memory timezone data (when `TOBASA_BUILD_IN_MEMORY_TZDB` enabled)

**Loading Strategy:**

1. Configuration and TLS resource arrays are generated for every app-server build.
2. Templates and `wwwroot` files are embedded when `TOBASA_BUILD_IN_MEMORY_RESOURCES=ON`.
3. `app::Resource::get()` searches embedded arrays only; callers such as the static-file handler implement any disk fallback.


**CMake Build Options:**

- `TOBASA_BUILD_IN_MEMORY_RESOURCES=ON` – Embeds templates and `wwwroot` assets
- `TOBASA_BUILD_IN_MEMORY_TZDB=ON` – Builds timezone assets when the selected date implementation needs them

Embedded resources do not make the application self-contained; external
configuration, application data, database, and any non-embedded files are
still required.

Further Documentation See [doc/app_resource.md](doc/app_resource.md)

### Utilities (`app_util.cpp`, `app_common.cpp`)

Common helper functions for date/time handling, string processing, JSON manipulation, and other frequently-used operations.

## Development & Testing

### Integrated Test Modules

The `TOBASA_BUILD_TESTS` CMake option controls the app-server test sources and
controller endpoints. CMake defines `TOBASA_USE_TESTS_MODULE` for the target
when those sources are included.

#### SQL Database Tests (`src/test/`)

Multi-driver database compatibility tests validating `tobasasql` abstraction across all supported databases:

- **SQLite** (`test_sql_sqlite_defs.h`) – File-based database tests
- **MySQL/MariaDB** (`test_sql_mysql_defs.h`, `test_sql_odbc_mysql_defs.h`) – Native and ODBC connectors
- **PostgreSQL** (`test_sql_pgsql_defs.h`) – Native connector
- **MSSQL** (`test_sql_ado_defs.h`, `test_sql_odbc_mssql_defs.h`) – ADO.NET and ODBC connectors
- **DTO/JSON Serialization** (`test_sql_json_dto.h`) – Entity-to-JSON mapping validation

Individual SQL test endpoints are listed in
[`doc/endpoints.md`](doc/endpoints.md); there is no general `/test/sql` route.

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


## License

See [LICENSE](LICENSE) in this directory for licensing information.

## Further Documentation

- Main project README: See `README.md` at repository root
- Tobasa Libraries Documentation: Each library has its own documentation
- CMake Configuration: See `cmake/` directory

## Support & Contributing

For issues, questions, or contributions, please refer to the main project repository.
