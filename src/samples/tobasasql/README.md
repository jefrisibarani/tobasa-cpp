# TobasaSQL Sample

A database abstraction layer example using the Tobasa framework.

## Overview

This sample demonstrates how to use `tobasasql` for database operations. It shows how to connect to databases, execute queries, and manage data using the unified abstraction layer.

## Features

- Multi-database support (SQLite, MySQL, PostgreSQL, MSSQL)
- Connection pooling
- Parameterized queries
- Transaction support
- Entity mapping

## Building

The sample is built as part of the main build system:

```bash
cmake -B build
cmake --build build
```

The compiled executable will be in `_output/test_tobasasql/debug/`

## Running

```bash
./test_tobasasql
```

## Supported Databases

- SQLite - File-based, zero configuration
- MySQL/MariaDB - Popular relational database
- PostgreSQL - Enterprise-grade database
- MSSQL - SQL Server

## Configuration

Database connection is configured in `appsettings.json`:

## Features

- Execute SELECT, INSERT, UPDATE, DELETE queries
- Connection pooling for performance
- Parameter binding for security
- Transaction management

## License

See LICENSE file in the root directory.
