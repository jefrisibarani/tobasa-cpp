# TobasaSQL quick start

## Introduction

TobasaSQL is a lightweight C++ SQL abstraction layer. It lets an application
use the same high-level connection, parameter, query, and result APIs with
SQLite, PostgreSQL, MySQL/MariaDB, and Microsoft SQL Server. The ODBC and ADO
drivers in this project are implemented and tested specifically for Microsoft
SQL Server; they are not general-purpose ODBC/ADO database integrations.

TobasaSQL is deliberately not an ORM. SQL statements remain visible to the
application, while the library provides:

* driver-specific connection and parameter binding behind `SqlConnection`;
* parameterized SQL through `SqlQuery` and `DataType`;
* scalar, command, and navigable result operations;
* normalized column metadata and backend-native metadata access;
* optional SQL and execution logging.

The central types are templates over a selected driver:

| Type | Purpose |
| --- | --- |
| `sql::SqlConnection<Driver>` | Open/close a connection and execute SQL. |
| `sql::SqlQuery<Driver>` | Hold SQL plus typed parameters and execute it. |
| `sql::SqlResult<Driver>` | Navigate rows and read values/metadata. |
| `sql::SqlTable<Driver>` | Open and edit a table through the table helper API. |
| `sql::DataType` | Portable type requested for a parameter or reported for a column. |

The complete portable type and conversion reference is in
[`data_types.md`](data_types.md).

## Before you start

Build the repository with CMake as described in [`BUILD.md`](../../../BUILD.md).
The TobasaSQL CMake options are:

| Option | Backend |
| --- | --- |
| `TOBASA_SQL_USE_SQLITE` | SQLite; enabled by the TobasaSQL CMake file if no backend is selected. |
| `TOBASA_SQL_USE_PGSQL` | PostgreSQL through `libpq`. |
| `TOBASA_SQL_USE_MYSQL` | MySQL/MariaDB through the MariaDB C connector. |
| `TOBASA_SQL_USE_ODBC` | Microsoft SQL Server through the tested SQL Server ODBC driver configuration. |
| `TOBASA_SQL_USE_ADODB` | Microsoft SQL Server through ADO on MSVC/Windows; the source also requires `_MSC_VER`. |

The selected option controls which driver aliases are available. For example,
`SqliteDriver` is declared only when `TOBASA_SQL_USE_SQLITE` is defined.

## Two ways to use TobasaSQL

TobasaSQL can be used directly with a known backend or through its
configuration-driven connector layer. Both patterns use the same low-level SQL
APIs; the difference is when the backend is chosen.

### Direct typed connection

Use `sql::SqlConnection<Driver>` when the application knows its database
backend at compile time. This is the simplest approach and is suitable for
small tools, sample programs, tests, and fixed-backend applications.

```cpp
tbs::sql::SqlConnection<tbs::sql::SqliteDriver> connection;

if (connection.connect("Database=./app.db3;OpenCreate=True;"))
{
   connection.execute("CREATE TABLE IF NOT EXISTS items (id INTEGER)");
   auto value = connection.executeScalar("SELECT COUNT(*) FROM items");
   connection.disconnect();
}
```

The driver type selects the implementation, for example `SqliteDriver`,
`PgsqlDriver`, `MysqlDriver`, `OdbcDriver`, or `AdodbDriver`, when that driver
is enabled by CMake. SQL execution, typed parameters, and result handling are
available directly from the connection and related `SqlQuery` and `SqlResult`
types.

### Configuration-driven connector

Use `DatabaseConnector` or `DbServiceFactory` when the database configuration
should select the backend at runtime. This is the application-oriented path for
configured services, connection pools, and repository code.

`DatabaseConnector` selects the development or production settings, creates the
matching typed connection internally, applies logging options, connects, and
exposes connection, transaction, and service-creation operations.

`DbServiceFactory` builds on that connector and can create pooled or
non-pooled services:

```cpp
tbs::sql::DbServiceFactory factory;
factory.addConnectorOption("MainDb", connectorOptions);

auto service = factory.createService<MyService>("MainDb", true);
```

Use `true` for pooled service creation when connections should be acquired and
returned automatically, or `false` for a long-lived non-pooled connector.
The connector configuration contains the backend, connection string,
environment selection, and SQL logging settings. The configured backend must
also have its corresponding CMake driver enabled.

### Choosing an approach

| Approach | Best suited for | Backend selection |
| --- | --- | --- |
| Direct `SqlConnection<Driver>` | Small programs, tests, samples, fixed-backend applications | Compile time |
| `DatabaseConnector` / `DbServiceFactory` | Configured applications, services, repositories, connection pooling | Runtime configuration |

Both approaches ultimately use the same typed `SqlConnection<Driver>` APIs.
The connector layer adds lifecycle and configuration management; it does not
replace SQL with an ORM.

## Sample programs

The repository includes three focused examples in [`../samples/tobasasql`](../samples/tobasasql):

* [`simple.cpp`](../samples/tobasasql/simple.cpp) — direct typed-connection smoke test for enabled backends;
* [`connector.cpp`](../samples/tobasasql/connector.cpp) — runtime configuration via `DatabaseConnector`;
* [`dbservice.cpp`](../samples/tobasasql/dbservice.cpp) — service-layer example using `DbServiceFactory`.
* [`pool.cpp`](../samples/tobasasql/pool.cpp) — pooled-connection example using `DbServiceFactory`.

## Minimal SQLite program

The following is the smallest useful pattern for a **direct typed connection**.
It follows the setup performed by [`../samples/tobasasql/simple.cpp`](../samples/tobasasql/simple.cpp), which also uses direct typed connections for each enabled backend. This pattern does not use `DatabaseConnector` or `DbServiceFactory`.

```cpp
#include <iostream>
#include <tobasa/datetime.h>
#include <tobasa/logger.h>
#include <tobasasql/sql_connection.h>
#include <tobasasql/sql_driver.h>

int main()
{
   if (!tbs::DateTime::initTimezoneData())
      return 1;

   tbs::Logger::setTarget(new tbs::log::CoutLogSink());

   try
   {
      tbs::sql::SqlConnection<tbs::sql::SqliteDriver> connection;
      if (!connection.connect( "Database=./tbs_coba.db3;OpenCreate=True;OpenMemory=False;"))
      {
         return 1;
      }

      std::cout << "Current time: "
                << connection.executeScalar("SELECT CURRENT_TIMESTAMP")
                << "\n";
      std::cout << "Backend: " << connection.versionString() << "\n";

      connection.disconnect();
   }
   catch (const std::exception& ex)
   {
      std::cerr << "SQL error: " << ex.what() << "\n";
      return 1;
   }
}
```

`connect()` returns `true` on a successful connection. SQL failures throw a
`SqlException` (which derives from `std::exception`); a connection object also
disconnects itself when it is destroyed while connected.

The SQLite connection string in the sample uses a file database and
`OpenCreate=True`. The sample also includes a password in its SQLite string
because that build can use the bundled encrypted SQLite variant. Do not copy
passwords or production credentials into source code.

## Execute commands and scalars

Use `execute()` for commands where the affected-row count matters, and
`executeVoid()` when a Boolean success result is sufficient. Use
`executeScalar()` when only the first column of the first row is needed. The
scalar API returns `std::string`, including for numeric and date values.

```cpp
int affected = connection.execute(
   "CREATE TABLE IF NOT EXISTS people ("
   "id INTEGER PRIMARY KEY, name TEXT NOT NULL)");

connection.execute(
   "INSERT INTO people (name) VALUES ('Ada')");

std::string name = connection.executeScalar(
   "SELECT name FROM people ORDER BY id LIMIT 1");
```

Do not build SQL by concatenating user input. Use typed parameters for values.

## Parameterized SQL with `SqlQuery`

`SqlQuery` stores parameters in the order they are added. By default, its SQL
uses named placeholders such as `:id`; TobasaSQL rewrites those placeholders
to the native placeholder syntax of the selected backend. The sample uses
`ParameterStyle::native` for MySQL because its SQL already contains `?`
placeholders.

```cpp
#include <tobasasql/sql_query.h>

tbs::sql::SqlQuery<tbs::sql::SqliteDriver> query(
   connection,
   "SELECT id, name FROM people WHERE id = :id");

query.addParam("id", tbs::sql::DataType::integer, 1);
std::string result = query.executeScalar();
```

The parameter `DataType` controls the backend type used for binding. Common
examples are:

```cpp
query.addParam("id",      tbs::sql::DataType::integer, 1);
query.addParam("enabled", tbs::sql::DataType::boolean, true);
query.addParam("score",   tbs::sql::DataType::float8,  3.14);
query.addParam("name",    tbs::sql::DataType::varchar, std::string("Ada"));
```

For strings and binary values, pass a size when the backend needs an explicit
parameter size. `DataType::numeric` is represented by decimal text, and
`DataType::varbinary` is represented by binary bytes in the portable API. See
[`data_types.md`](data_types.md) for backend-specific exceptions.

To use native placeholders instead of named placeholders, construct the query
with `ParameterStyle::native` and add parameters in placeholder order:

```cpp
tbs::sql::SqlQuery<tbs::sql::SqliteDriver> nativeQuery(
   connection,
   "SELECT name FROM people WHERE id = ?",
   tbs::sql::ParameterStyle::native);
nativeQuery.addParam("id", tbs::sql::DataType::integer, 1);
std::string nativeResult = nativeQuery.executeScalar();
```

## Read multiple rows

For a result set, call `SqlQuery::executeResult()` or construct a
`SqlResult` and call `runQuery()`. Check `isValid()` and `totalRows()`, then
navigate with `moveFirst()`/`moveNext()` and read by column name or zero-based
column index.

```cpp
auto resultSet = query.executeResult();
if (resultSet->isValid() && resultSet->totalRows() > 0)
{
   resultSet->moveFirst();

   while (!resultSet->isEof())
   {
      std::string currentName = resultSet->getStringValue("name");
      std::cout << currentName << "\n";
      resultSet->moveNext();
   }
}
```

Typed access is available through `get<T>()` when the backend variant contains
that exact alternative:

```cpp
auto value = resultSet->getVariantValue("id");
int64_t id = resultSet->get<int64_t>("id");
```

The portable convenience getters include `getStringValue`, `getLongValue`,
`getLongLongValue`, `getDoubleValue`, `getBoolValue`, and
`getDateTimeValue`. `getStringValue(column, valueIfNull)` can supply a value
for a NULL field. A typed `get<T>()` call must match the actual backend
variant alternative; use `getVariantValue()` or a convenience getter when
backend representation differs.

Column metadata is available through `columnDataType`,
`columnNativeTypeStr`, `columnNativeFullTypeStr`, `columnDefinedSize`, and
`columnTypeClass`. Native-to-portable conversion can be lossy, so use the
native metadata when an application needs backend-specific precision,
unsignedness, time-zone information, or a type not present in `DataType`.

## Backend-independent service interfaces

When using `DbServiceFactory`, a service can expose a non-templated interface
to the rest of the application while keeping the backend-specific SQL code in
a templated implementation. This is the pattern shown in
[`../samples/tobasasql/dbservice.cpp`](../samples/tobasasql/dbservice.cpp).

The public service interface derives from `sql::SqlServiceBase` and declares
only operations that application code needs:

```cpp
class UserServiceBase : public tbs::sql::SqlServiceBase
{
public:
   virtual std::vector<std::string> getUserNames() = 0;
   virtual bool addUser(const std::string& userName, int userLevel) = 0;
};
```

The implementation remains templated because it stores and uses a concrete
`SqlConnection<SqlDriverType>`:

```cpp
template <typename SqlDriverType>
class UserService : public UserServiceBase
{
private:
   using SqlConnection = tbs::sql::SqlConnection<SqlDriverType>;
   SqlConnection& _connection;

public:
   explicit UserService(SqlConnection& connection)
      : _connection(connection) {}

   std::vector<std::string> getUserNames() override;
   bool addUser(const std::string& userName, int userLevel) override;
};
```

Pass the service template to `createService()`. The configured connector
selects the driver and internally creates the matching specialization, such as
`UserService<SqliteDriver>` or `UserService<PgsqlDriver>`:

```cpp
tbs::sql::SqlServicePtr service =
   factory.createService<UserService>("MainDb", false);

auto userService =
   std::static_pointer_cast<UserServiceBase>(service);

userService->addUser("Ada", 1);
auto names = userService->getUserNames();
```

The caller does not need to name the driver when calling service methods. The
driver template is still required at the factory boundary so the factory can
construct the correct typed implementation for the selected backend.

## Connection strings and backend syntax

TobasaSQL does not impose a single portable connection-string format across all
backends. The application passes the connection string through to the selected
backend driver almost unchanged. In other words, the library expects the string
syntax required by the underlying database client or provider, while the
TobasaSQL API is responsible for opening the connection and executing SQL.

The sample includes corresponding blocks for all enabled drivers:

| Driver | Driver type | Connection string form used by the sample |
| --- | --- | --- |
| SQLite | `SqliteDriver` | `Database=...;OpenCreate=True;OpenMemory=False;` |
| PostgreSQL | `PgsqlDriver` | `dbname=... user=... password=... hostaddr=... port=...` |
| ADO | `AdodbDriver` | `Provider=...;Server=...;Database=...;Uid=...;Pwd=...;` |
| ODBC | `OdbcDriver` | `Driver={...};Server=...;Database=...;Uid=...;Pwd=...;` |
| MySQL/MariaDB | `MysqlDriver` | `Database=...;User=...;Password=...;Server=...;Port=...` |

A few important details apply across all drivers:

* The string is passed directly to the selected backend library; TobasaSQL does
  not rewrite or normalize it.
* Use the exact syntax and option names required by that backend and the
  installed client library.
* The user name, password, hostname, and database name are backend-specific and
  must match the server configuration.
* If a driver uses a provider or DSN name, the value must be the one known to
  the local installation.

The ODBC and ADO examples target Microsoft SQL Server and are not documented or
tested here for other database engines. For ADO on Windows, call
`CoInitializeEx` before using ADO and `CoUninitialize` after the connection is
finished.

## Logging and errors

The sample installs `CoutLogSink` before creating connections. Per-connection
SQL logging can be enabled with:

```cpp
connection.setLogSqlQuery(true);
connection.setLogSqlQueryInternal(true);
connection.setLogExecuteStatus(true);
```

These settings can expose SQL text and parameter activity, so enable them
carefully in production. Catch `std::exception` around connection and query
operations, and log or handle `SqlException` without leaking connection
credentials.

## Next steps

* Read [`data_types.md`](data_types.md) before designing portable schemas.
* Use `SqlQuery` parameters for all external values.
* Use `SqlResult` when a query returns rows and inspect `columnDataType()` when
  converting values.
* Use `SqlTable` only when its table-editing workflow fits the application;
  direct SQL remains the primary TobasaSQL model.
