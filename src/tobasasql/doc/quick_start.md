# TobasaSQL quick start

## Introduction

TobasaSQL is a small C++ SQL library. It gives your application the same main
connection, parameter, query, and result APIs for SQLite, PostgreSQL,
MySQL/MariaDB, and Microsoft SQL Server.

The ODBC and ADO drivers in this project are built and tested for Microsoft
SQL Server. They are not general-purpose ODBC or ADO integrations for every
database.

TobasaSQL is not an ORM. You write the SQL yourself. The library handles the
backend-specific parts of connections, parameters, results, metadata, and
optional logging.

The main types are templates that use a selected driver:

| Type | Purpose |
| --- | --- |
| `sql::SqlConnection<Driver>` | Connect, disconnect, and run SQL. |
| `sql::SqlQuery<Driver>` | Store SQL and typed parameters, then execute it. |
| `sql::SqlResult<Driver>` | Move through rows and read values or metadata. |
| `sql::SqlTable<Driver>` | Open and edit a table with the table helper API. |
| `sql::DataType` | Portable type used for a parameter or reported for a column. |

See [`data_types.md`](data_types.md) for the complete type and conversion
reference.

## Before you start

Build the repository with CMake as described in [`BUILD.md`](../../../BUILD.md).
Choose the TobasaSQL backend options you need:

| Option | Backend |
| --- | --- |
| `TOBASA_SQL_USE_SQLITE` | SQLite. The TobasaSQL CMake file enables it when no backend is selected. |
| `TOBASA_SQL_USE_PGSQL` | PostgreSQL through `libpq`. |
| `TOBASA_SQL_USE_MYSQL` | MySQL/MariaDB through the MariaDB C connector. |
| `TOBASA_SQL_USE_ODBC` | Microsoft SQL Server through the tested SQL Server ODBC setup. |
| `TOBASA_SQL_USE_ADODB` | Microsoft SQL Server through ADO on MSVC/Windows; the source also requires `_MSC_VER`. |

The CMake option controls which driver types are available. For example,
`SqliteDriver` exists only when `TOBASA_SQL_USE_SQLITE` is defined.

## Two ways to use TobasaSQL

There are two common ways to use the library. Use a typed connection when the
backend is fixed. Use the connector layer when configuration should choose the
backend at runtime.

### Direct typed connection

Use `sql::SqlConnection<Driver>` when the database type is known at compile
time. This is usually the easiest choice for a small tool, test, sample, or
application that always uses one backend.

```cpp
tbs::sql::SqlConnection<tbs::sql::SqliteDriver> connection;

if (connection.connect("Database=./app.db3;OpenCreate=True;"))
{
   connection.execute("CREATE TABLE IF NOT EXISTS items (id INTEGER)");
   auto value = connection.executeScalar("SELECT COUNT(*) FROM items");
   connection.disconnect();
}
```

The driver type selects the implementation, such as `SqliteDriver`,
`PgsqlDriver`, `MysqlDriver`, `OdbcDriver`, or `AdodbDriver`, when that driver
is enabled by CMake.

The connection, `SqlQuery`, and `SqlResult` types then give you the same SQL,
parameter, and result operations for that backend.

### Configuration-driven connector

Use `DatabaseConnector` or `DbServiceFactory` when configuration should choose
the backend at runtime. This is a better fit for configured applications,
services, repositories, and connection pools.

`DatabaseConnector` reads the development or production settings, creates the
matching typed connection, applies SQL logging options, connects, and exposes
connection, transaction, and service operations.

`DbServiceFactory` builds on it and can create pooled or non-pooled services:

```cpp
tbs::sql::DbServiceFactory factory;
factory.addConnectorOption("MainDb", connectorOptions);

auto service = factory.createService<MyService>("MainDb", true);
```

Use `true` when the service should acquire and return pooled connections
automatically. Use `false` for a long-lived non-pooled connector.

The connector settings contain the backend, connection string, selected
environment, and SQL logging options. The matching CMake driver must also be
enabled.

### Choosing an approach

| Approach | Best suited for | Backend selection |
| --- | --- | --- |
| Direct `SqlConnection<Driver>` | Small programs, tests, samples, and fixed-backend applications | Compile time |
| `DatabaseConnector` / `DbServiceFactory` | Configured applications, services, repositories, and connection pools | Runtime configuration |

Both approaches eventually use a typed `SqlConnection<Driver>`. The connector
layer adds configuration and connection lifetime handling; it does not turn
SQL into an ORM.

## Sample programs

The repository has four focused examples in
[`../../samples/tobasasql`](../../samples/tobasasql):

* [`simple.cpp`](../../samples/tobasasql/simple.cpp) - direct typed-connection test for enabled backends;
* [`connector.cpp`](../../samples/tobasasql/connector.cpp) - runtime configuration with `DatabaseConnector`;
* [`dbservice.cpp`](../../samples/tobasasql/dbservice.cpp) - service-layer example with `DbServiceFactory`;
* [`pool.cpp`](../../samples/tobasasql/pool.cpp) - pooled-connection example with `DbServiceFactory`.

## Minimal SQLite program

This is a small working example for a **direct typed connection**. It follows
the setup in [`../../samples/tobasasql/simple.cpp`](../../samples/tobasasql/simple.cpp),
which also uses direct typed connections for each enabled backend.

This example does not use `DatabaseConnector` or `DbServiceFactory`.

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

`connect()` returns `true` when the connection succeeds. SQL failures throw a
`SqlException`, which derives from `std::exception`. A connected connection
also disconnects when its object is destroyed.

The sample uses a file database and `OpenCreate=True`. It also includes a
password in its SQLite string because that build can use the bundled encrypted
SQLite variant. Do not put passwords or production credentials in source code.

## Execute commands and scalars

Use `execute()` when you need the affected-row count. Use `executeVoid()` when
a Boolean success result is enough. Use `executeScalar()` when you need only
the first column from the first row. The scalar API returns a `std::string`,
even for numbers and dates.

```cpp
int affected = connection.execute(
   "CREATE TABLE IF NOT EXISTS people ("
   "id INTEGER PRIMARY KEY, name TEXT NOT NULL)");

connection.execute(
   "INSERT INTO people (name) VALUES ('Ada')");

std::string name = connection.executeScalar(
   "SELECT name FROM people ORDER BY id LIMIT 1");
```

Do not build SQL by joining user input into a string. Use typed parameters.

## Parameterized SQL with `SqlQuery`

`SqlQuery` keeps parameters in the order you add them. By default, write named
placeholders such as `:id`; TobasaSQL changes them to the syntax required by
the selected backend.

The sample uses `ParameterStyle::native` for MySQL because the SQL already
uses `?` placeholders.

```cpp
#include <tobasasql/sql_query.h>

tbs::sql::SqlQuery<tbs::sql::SqliteDriver> query(
   connection,
   "SELECT id, name FROM people WHERE id = :id");

query.addParam("id", tbs::sql::DataType::integer, 1);
std::string result = query.executeScalar();
```

The `DataType` tells the backend which type to use for binding. Common types
look like this:

```cpp
query.addParam("id",      tbs::sql::DataType::integer, 1);
query.addParam("enabled", tbs::sql::DataType::boolean, true);
query.addParam("score",   tbs::sql::DataType::float8,  3.14);
query.addParam("name",    tbs::sql::DataType::varchar, std::string("Ada"));
```

For strings and binary values, give a size when the backend needs an explicit
parameter size. `DataType::numeric` uses decimal text, while
`DataType::varbinary` uses binary bytes in the portable API. See
[`data_types.md`](data_types.md) for backend-specific details.

To use native placeholders, construct the query with
`ParameterStyle::native` and add parameters in placeholder order:

```cpp
tbs::sql::SqlQuery<tbs::sql::SqliteDriver> nativeQuery(
   connection,
   "SELECT name FROM people WHERE id = ?",
   tbs::sql::ParameterStyle::native);
nativeQuery.addParam("id", tbs::sql::DataType::integer, 1);
std::string nativeResult = nativeQuery.executeScalar();
```

## Read multiple rows

For several rows, call `SqlQuery::executeResult()` or create a `SqlResult` and
call `runQuery()`. Check `isValid()` and `totalRows()`, then move through the
rows with `moveFirst()` and `moveNext()`.

You can read a column by name or by its zero-based index.

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

You can also request a typed value when the backend variant contains the exact
type:

```cpp
auto value = resultSet->getVariantValue("id");
int64_t id = resultSet->get<int64_t>("id");
```

Convenience getters include `getStringValue`, `getLongValue`,
`getLongLongValue`, `getDoubleValue`, `getBoolValue`, and
`getDateTimeValue`. Use `getStringValue(column, valueIfNull)` to provide a
value for a NULL column.

A typed `get<T>()` call must match the backend's actual variant type. Use
`getVariantValue()` or a convenience getter when that type may differ between
backends.

Column information is available through `columnDataType`,
`columnNativeTypeStr`, `columnNativeFullTypeStr`, `columnDefinedSize`, and
`columnTypeClass`.

Portable conversion can lose backend-specific details. Use native metadata when
you need exact precision, unsigned values, time-zone information, or a type
that is not in `DataType`.

## Backend-independent service interfaces

With `DbServiceFactory`, you can give the rest of your application a normal,
non-templated service interface while keeping backend-specific SQL in a
templated implementation. See
[`../../samples/tobasasql/dbservice.cpp`](../../samples/tobasasql/dbservice.cpp)
for the same pattern.

The public interface derives from `sql::SqlServiceBase` and contains only the
operations the application needs:

```cpp
class UserServiceBase : public tbs::sql::SqlServiceBase
{
public:
   virtual std::vector<std::string> getUserNames() = 0;
   virtual bool addUser(const std::string& userName, int userLevel) = 0;
};
```

The implementation is still templated because it stores a concrete
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

Pass the service template to `createService()`. The connector chooses the
driver and creates the right specialization, such as
`UserService<SqliteDriver>` or `UserService<PgsqlDriver>`:

```cpp
tbs::sql::SqlServicePtr service =
   factory.createService<UserService>("MainDb", false);

auto userService =
   std::static_pointer_cast<UserServiceBase>(service);

userService->addUser("Ada", 1);
auto names = userService->getUserNames();
```

Callers do not need to name the database driver when using the service. The
driver template is needed at the factory boundary so the factory can create
the correct implementation.

## Connection strings and backend syntax

TobasaSQL does not use one connection-string format for every backend. It passes
the string to the selected database client or provider with little or no
rewriting.

Use the syntax required by the database client. TobasaSQL handles opening the
connection and running SQL, not converting one backend's connection syntax to
another's.

The samples use these forms:

| Driver | Driver type | Connection string form used by the sample |
| --- | --- | --- |
| SQLite | `SqliteDriver` | `Database=...;OpenCreate=True;OpenMemory=False;` |
| PostgreSQL | `PgsqlDriver` | `dbname=... user=... password=... hostaddr=... port=...` |
| ADO | `AdodbDriver` | `Provider=...;Server=...;Database=...;Uid=...;Pwd=...;` |
| ODBC | `OdbcDriver` | `Driver={...};Server=...;Database=...;Uid=...;Pwd=...;` |
| MySQL/MariaDB | `MysqlDriver` | `Database=...;User=...;Password=...;Server=...;Port=...` |

Keep these points in mind:

* The selected backend library receives the connection string directly.
* Use the option names and syntax required by that backend and installed
  client library.
* The user, password, host, and database name must match the server setup.
* When a driver uses a provider or DSN name, use the name installed locally.

The ODBC and ADO examples target Microsoft SQL Server. They are not documented
or tested here with other database engines.

On Windows, call `CoInitializeEx` before using ADO and `CoUninitialize` after
the connection is finished.

## Logging and errors

The samples install `CoutLogSink` before creating connections. You can turn on
per-connection SQL logging like this:

```cpp
connection.setLogSqlQuery(true);
connection.setLogSqlQueryInternal(true);
connection.setLogExecuteStatus(true);
```

SQL logging can expose SQL text and parameter activity. Use it carefully in
production.

Catch `std::exception` around connection and query calls. Handle
`SqlException` without writing connection passwords or other credentials to
the log.

## Next steps

* Read [`data_types.md`](data_types.md) before designing portable schemas.
* Use `SqlQuery` parameters for every value that comes from outside the program.
* Use `SqlResult` for queries that return rows, and check `columnDataType()` when converting values.
* Use `SqlTable` only when its table-editing workflow fits your application. Direct SQL is still the main TobasaSQL approach.
