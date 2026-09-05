# TobasaSQL

TobasaSQL is a lightweight C++ SQL abstraction layer for connecting to SQLite,
PostgreSQL, MySQL/MariaDB, and Microsoft SQL Server through a unified API. It
is designed for direct SQL access without ORM overhead, while still providing
common database features such as parameter binding, result iteration, logging,
and service-oriented configuration.

## Quick start

For a practical walkthrough, start with [doc/quick_start.md](doc/quick_start.md).
It covers the basic connection patterns, backend-specific connection strings,
parameterized queries, and sample usage.

## Usage patterns

TobasaSQL supports two main ways to work with databases:

- Direct typed connections with `sql::SqlConnection<Driver>` when the backend is
  known at compile time.
- Runtime-selected configuration through `DatabaseConnector` and
  `DbServiceFactory` when the backend is chosen from configuration.

Both patterns use the same low-level SQL API surface; the connector layer simply
adds configuration and lifecycle management.

## Key features

- Unified API across supported database backends
- Parameterized SQL with type-aware bindings
- Query execution, scalar access, and result iteration
- Optional SQL and execution logging
- Transaction support and service-oriented database access
- Minimal abstraction overhead with direct SQL control

## Supported databases

| Database | Driver | Notes |
| --- | --- | --- |
| SQLite | sqlite3 | Embedded and development-friendly use |
| PostgreSQL | libpq | Server-side relational workloads |
| MySQL/MariaDB | libmariadb | Web and cloud deployment use |
| Microsoft SQL Server | ODBC/ADO | Tested and implemented specifically for SQL Server |

## Build and dependencies

The library is built as part of the main project build.

### Required dependency

- Tobasa core library

### Database-specific dependencies

- SQLite: bundled `sqlite3_mc` support
- PostgreSQL: `libpq`
- MySQL/MariaDB: `libmariadb`
- Microsoft SQL Server: ODBC or ADO on Windows
- zlib: required for the bundled MariaDB connector build path

## Architecture summary

TobasaSQL keeps the abstraction thin:

- direct SQL remains the primary programming model;
- query parameters are strongly typed when possible;
- backend-specific behavior is isolated at the driver layer;
- application code can work with either a fixed driver or a configured runtime
  backend.

## License

GNU LESSER GENERAL PUBLIC LICENSE