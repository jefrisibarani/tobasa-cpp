# Tobasa SQL Library
C++ library to use many SQL databases with one interface.

## Overview
Tobasa SQL is a small, focused library that helps C++ programs connect to
and run SQL queries on different database systems without changing much code.
It is not an ORM; you write SQL and handle results yourself.

## Features
 - A common connection and query API across backends
 - Logging of SQL queries and execution status
 - Lightweight and easy to add to C++ projects

## Supported databases
 - SQLite
 - PostgreSQL
 - MariaDB / MySQL
 - MS SQL (via ODBC or AdoDB on Windows)

## Dependencies
 - SQLite client library
 - libpq (PostgreSQL client)
 - libmariadb (MariaDB/MySQL client)
 - ODBC / unixODBC for ODBC backends
 - zlib (for libmariadb)
 - AdoDB (Windows builds only)

## License
GNU LESSER GENERAL PUBLIC LICENSE