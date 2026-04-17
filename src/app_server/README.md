# Tobasa Web Service Application

A comprehensive sample web service application demonstrating the integration of Tobasa libraries into a production-ready server.

## Overview

The **app_server** (webservice) is an example application that showcases how to build a full-featured web service using the Tobasa framework. It integrates multiple Tobasa components to provide HTTP/HTTPS capabilities, REST routing, database connectivity, and more.

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

- Loads configuration
- Initializes logging via `tobasa`
- Sets up the HTTP server with `tobasahttp`
- Registers routes and controllers
- Connects to the database via `tobasasql`
- Starts the server

### Controllers & Routing (`core/`)

Dynamic request handlers organized by functionality. Routes are matched using configurable matchers (regex or wildcard patterns).

### Middleware Stack (`middleware/`)

Request/response processing chain for cross-cutting concerns like:

- Authentication/authorization
- Logging and monitoring
- CORS handling
- Request validation

### Database Repository (`db_repo_app.h`)

Data access layer providing queries and operations on the connected database.

### Resource Management (`app_resource.cpp`)

Handles loading static assets (views, configuration, TLS certificates) from either embedded binary resources or filesystem.

### Utilities (`app_util.cpp`, `app_common.cpp`)

Common helper functions for date/time handling, string processing, JSON manipulation, and other frequently-used operations.

## Development & Testing

### Unit Tests
Located in `src/test/`:


### WebSocket Tests / Chat app
WebSocket-specific tests in `src/test_ws/`:



### Database Migrations

Database schema changes are managed in `src/db_migrations/`. The application automatically applies pending migrations on startup or via CLI.

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
