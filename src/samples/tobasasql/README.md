# TobasaSQL Sample

A database abstraction layer example using the Tobasa framework.

## Sample programs

- `simple.cpp` - direct typed-connection smoke test. It opens a connection for
  each enabled backend, runs a small query, and prints the backend version.
- `connector.cpp` - configuration-driven connector example. It uses
  `DatabaseConnector` and `ConnectorOption` to select the active database
  configuration at runtime.
- `dbservice.cpp` - service-oriented example. It uses `DbServiceFactory` to
  create a typed service, add sample records, and exercise transactions.
- `pool.cpp` - pooled-connection example. It configures a connection pool,
  acquires a few connectors, runs work through them, and shows that released
  connections are returned to the pool automatically.
- `test_mysql.cpp` - MySQL-specific behavior check. It validates MySQL/MariaDB
  type conversion, bit handling, and sample database interactions.

## Building

The sample is built as part of the main CMake build. The resulting executable is
written to the active output directory for the current configuration.

## Running

```bash
./test_tobasasql
./test_tobasasql_connector
./test_tobasasql_dbsvc
./test_tobasasql_pool
./test_mysql
```