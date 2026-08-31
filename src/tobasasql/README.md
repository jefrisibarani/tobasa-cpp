# Tobasa SQL Library

A lightweight C++ database abstraction layer supporting multiple SQL databases with a unified interface.

## Overview

Tobasa SQL provides a thin, efficient abstraction layer for SQL database operations. It enables applications to work with different databases (SQLite, PostgreSQL, MySQL, MSSQL) using the same code. The library focuses on core database operations—connections, queries, transactions—without ORM overhead.

## Features

**Unified API:**
- Single interface for all supported database backends
- Query execution with consistent error handling
- Result set management and iteration
- Connection pooling for performance

**Query Capabilities:**
- Parameterized queries for SQL injection prevention
- Support for SELECT, INSERT, UPDATE, DELETE operations
- Transaction support (begin, commit, rollback)
- Prepared statement support
- Batch query execution

**Developer Experience:**
- Minimal configuration required
- Direct SQL query control
- Query logging and diagnostics
- Type-safe result binding
- Easy integration into existing C++ projects

## Building

The library is built as part of the main build system:

```bash
cmake -B build
cmake --build build
```

Optional database drivers can be enabled:
```bash
cmake -B build -DENABLE_POSTGRESQL=ON -DENABLE_MYSQL=ON
cmake --build build
```

## Supported Databases

| Database | Driver | Use Case |
|----------|--------|----------|
| **SQLite** | Built-in | Development, embedded deployments, single-file databases |
| **PostgreSQL** | libpq | Enterprise systems, advanced features, concurrency |
| **MySQL/MariaDB** | libmariadb | Web applications, cloud deployments, compatibility |
| **MS SQL** | ODBC/AdoDB | Windows environments, SQL Server integration |

## Configuration

Database connections are configured in `appsettings.json`:


## Architecture

**No ORM overhead:**
- Write SQL directly
- Full control over queries
- Minimal abstractions
- Predictable performance

**Connection Management:**
- Connection pooling (configurable pool size)
- Automatic reconnection
- Timeout handling
- Resource cleanup

**Query Execution:**
- Parameterized query binding
- Results as typed containers
- Error propagation
- Query logging

## Performance Characteristics

- Lightweight abstraction (minimal overhead)
- Efficient connection pooling
- Direct SQL execution without query translation
- Suitable for both OLTP and OLAP workloads
- Configurable timeout and retry behavior

## Dependencies

**Required:**
- Tobasa core library

**Database-Specific:**
- SQLite - Built-in support
- PostgreSQL - libpq (PostgreSQL client library)
- MySQL/MariaDB - libmariadb (MariaDB client library)
- MSSQL - ODBC drivers or AdoDB (Windows)
- zlib - For compression support

## License

GNU LESSER GENERAL PUBLIC LICENSE