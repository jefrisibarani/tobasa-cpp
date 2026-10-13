# Using the TobasaSQL `Db*` API

The `Db*` classes let application code use TobasaSQL without naming a database
driver in its types. Choose a backend at runtime with `DbConnectionOptions`,
then use `DbConnection`, `DbCommand`, `DbResult`, and `DbTable` for normal SQL
work.

This is still a SQL library, not an ORM. You write SQL, choose parameter types,
and decide how transactions and errors should be handled. The wrappers provide
one API across the enabled backends; they do not make every backend behave
identically.

## A small example

This example uses an in-memory SQLite database. The connection string syntax is
specific to the selected backend.

```cpp
#include <iostream>
#include <stdexcept>
#include <string>

#include <tobasasql/db_api.h>

int main()
{
   using namespace tbs::sql;

   DbConnectionOptions options;
   options.backend = BackendType::sqlite;
   options.connectionString = "OpenMemory=True;";

   try
   {
      DbConnection connection(options);
      connection.execute(
         "CREATE TABLE people (id INTEGER PRIMARY KEY, name TEXT NOT NULL)");

      DbCommand insert(connection,
         "INSERT INTO people (name) VALUES (:name)");
      insert.addParam("name", DataType::varchar, std::string("Ada"), 32);
      insert.execute();

      DbCommand select(connection,
         "SELECT id, name FROM people WHERE name = :name");
      select.addParam("name", DataType::varchar, std::string("Ada"), 32);

      DbResult rows = select.executeQuery();
      if (!rows.isValid())
         return 1;

      rows.moveFirst();
      while (!rows.isEof())
      {
         std::cout << rows.getInt64("id") << ": "
                   << rows.getString("name") << "\n";
         rows.moveNext();
      }
   }
   catch (const std::exception& error)
   {
      std::cerr << "Database error: " << error.what() << "\n";
      return 1;
   }
}
```

You can also create a command with
`connection.createCommand(sql, style)`. The `DbCommand` constructor and the
connection factory use the same connection state.

## Choose the `Db*` API when

Use `DbConnection` when your application chooses a backend from configuration,
or when a component should not expose driver types in its public interface.
`BackendType` selects the backend at runtime. The matching driver must still be
enabled in the build.

If the backend is fixed and you need driver-specific APIs, use the templated
`SqlConnection<Driver>`, `SqlQuery<Driver>`, and related types instead. See the
[quick start](quick_start.md) for both approaches.

## Opening a connection

Set the backend and the connection string in `DbConnectionOptions`:

```cpp
DbConnectionOptions options;
options.backend = BackendType::sqlite;
options.connectionString = "Database=./app.db3;OpenCreate=True;";
options.logSqlQuery = false;

DbConnection connection(options);
```

The connection string is passed to the selected backend. Use that backend's
syntax; a SQLite connection string is not valid for PostgreSQL, MySQL, ODBC, or
ADO. ODBC and ADO support in this project is aimed at Microsoft SQL Server.

The options also contain `password`, `securitySalt`, and
`connectionStringIsComplete` for connection-string handling, and logging
settings. Do not put credentials in source code or log output. Use
`setLogSqlQuery()`, `setLogSqlQueryInternal()`, and
`setLogExecuteStatus()` when per-connection logging needs to change after
opening.

Constructing `DbConnection(options)` opens the connection immediately. If it
cannot connect, construction throws; it does not return an empty connection.
The default constructor creates an empty handle. `operator bool()` tells you
whether a handle contains a connection. An empty handle reports neutral values
for several getters, such as `BackendType::unknown` and an empty database name.

`execute()` returns the affected-row count, or `-1` when the handle is empty.
`executeVoid()` and transaction methods return `false` for an empty handle.
`executeScalar()` throws `std::logic_error` on an empty handle. Creating a
command or table from an empty connection also throws `std::logic_error`.
Backend and SQL errors can throw from the underlying driver; catch
`std::exception` at an application boundary when appropriate.

## Commands and parameters

`DbCommand` represents one SQL statement and its current parameter list. Create
one from a connection in either of these ways:

```cpp
auto command = connection.createCommand(
   "UPDATE people SET name = :name WHERE id = :id");

DbCommand another(connection,
   "SELECT name FROM people WHERE id = :id");
```

The default parameter style is `ParameterStyle::named`. Write placeholders as
`:name`; the library rewrites them for the selected backend. If the SQL already
uses native placeholders, pass `ParameterStyle::native`:

```cpp
auto command = connection.createCommand(
   "SELECT name FROM people WHERE id = ?",
   ParameterStyle::native);
```

Add parameters with `addParam()` or `parameter()`:

```cpp
command.addParam("id", DataType::integer, int32_t{7});

DbParameter parameter;
parameter.name = "name";
parameter.type = DataType::varchar;
parameter.value = std::string("Grace");
parameter.size = 32;
   command.parameter(parameter);
```

Parameters are bound in order, not matched or reordered by their names. For
named SQL, add them in the order their placeholders appear in the SQL. For
native SQL, add them in the order expected by the backend's placeholder
positions. A parameter name is metadata for most drivers; ADO and some helper
APIs may use it more directly. Parameter direction and size are also passed to
the driver, but support for particular parameter features depends on that
driver.

Use `DataType` to describe the database type and a compatible value in
`DbValue`. `DbValue` can hold null (`std::monostate`), booleans, fixed-width
integer types, floating-point values, strings, and byte vectors. The
[data type guide](data_types.md) describes portable types and backend
differences.

### Execute and reuse a command

Choose the operation that matches the result you need:

| Call | Result |
| --- | --- |
| `execute()` | Affected-row count as `int`. |
| `executeVoid()` | Boolean success result. |
| `executeScalar()` | First column of the first row as `std::string`. |
| `executeQuery()` | A `DbResult` for row-oriented access. |

The scalar result is text even when the SQL value is numeric. It is not a way
to distinguish SQL `NULL` from every possible empty-text result; use
`executeQuery()` and `DbResult::isNull()` when that distinction matters.

`reset()` clears the command's current parameters and resets its execution
state. Add the parameters again before executing it again. `close()` closes the
backend command resources; it does not turn the C++ handle into a reusable
copy. Commands are move-only. An empty or moved-from command throws
`std::logic_error` when you try to bind or execute it. `reset()` and `close()`
are harmless on an empty handle, and `affectedRows()` returns zero there.

The parameter binding rules are covered in more detail in
[`sql_parameter.md`](sql_parameter.md) and
[`prepared_statement.md`](prepared_statement.md).

## Reading results

`executeQuery()` returns a move-only `DbResult`. It defaults to
`cacheData = true` and `openTable = false`; those options are passed to the
underlying result implementation. Check `isValid()` before reading, then move
the cursor and check `isEof()`:

```cpp
DbResult rows = command.executeQuery();
if (rows.isValid())
{
   rows.moveFirst();
   while (!rows.isEof())
   {
      const auto id = rows.getInt64(0);
      const auto name = rows.getString("name");
      rows.moveNext();
   }
}
```

Column indexes are zero-based. Use `columnNames()`, `columnName()`, and
`columnNumber()` to inspect names; `columnNumber()` returns `-1` if the name is
not found. Check names and result validity before calling typed getters.

Use `getValue()` when you want a `DbValue`, or a typed getter such as
`getInt64()`, `getBool()`, `getDouble()`, and `getDateTime()` when the value's
meaning is known. `getString(column, fallback)` returns the fallback for SQL
`NULL`; `isNull()` lets you test `NULL` separately. Column metadata methods
report portable type information as well as native type names and codes.
Portable types do not preserve every backend-specific detail.

On an empty result, status and metadata methods return neutral values, such as
`ResultStatus::unknown`, zero counts, or an empty name. `isEof()` and
`isBof()` return `true`, and `isNull()` returns `true`. `getValue()` returns a
default `DbValue`. String and typed getters throw `std::logic_error` if there
is no result object. These defaults are for safe handle inspection, not a
substitute for checking `isValid()` and column positions.

## Editing rows with `DbTable`

`DbTable` is a cached table-editing helper. It is useful for small editing
workflows, but direct SQL with `DbCommand` is usually clearer for application
queries and bulk updates.

Create the table view and call `init()` to load its schema and rows:

```cpp
DbTable people(connection, "people", {}, false);
people.init();

if (people.isValid() && !people.isEmpty())
{
   people.moveFirst();
   people.setValue("name", std::string("Grace"));
   if (!people.save())
      throw std::runtime_error("Could not save the row");
}
```

The constructor arguments are the connection, target table name, optional SQL
query, and read-only flag. With an empty query, the table helper selects from
the named table. The default is read-only. Passing `false` requests editing;
it does not make every query or schema editable. Views and tables without the
metadata needed for row updates can still be read-only, and key or generated
column rules can prevent individual edits. Test the exact schema and backend
you deploy.

`setValue()` changes the table's in-memory row. `save()` writes the last
modified row; it is not a general batch-save operation for every row that may
have been edited. `appendRows()` adds rows to the in-memory table. Check
`save()` after setting the values for a row, and use an explicit connection
transaction if several SQL operations must succeed or fail together.

Deletion has different timing: `deleteRows(position, count)` executes deletes
against the database while it runs. It is not deferred until `save()`. The
return value is `true` if at least one row was deleted, and
`lastSuccessfullyDeletedRows()` reports the count from the last call. Use a
transaction if deletion must be atomic with other work.

The table also provides cursor movement, row and column counts, column
metadata, and paging controls. The underlying table helper starts with a page
size of 10; call `setPageSize()` to choose another size. `setupLookup()` can
attach key/display values to a column. `DbLookup` exposes these as parallel
`keys` and `values` vectors and provides lookup helpers. A missing lookup key
or value returns an empty string; use `indexOfKey()` or `indexOfValue()` when
you need to distinguish a miss from an empty entry.

On an empty `DbTable`, `isValid()` is false and many getters return neutral
values. `setValue()` throws `std::logic_error` if there is no table object.
After `init()`, still check `isValid()` before using column positions or editing
rows.

## Transactions

Transactions belong to the connection. Start, commit, and roll back them with
`startTransaction()`, `commitTransaction()`, and `rollbackTransaction()`:

```cpp
if (!connection.startTransaction())
   throw std::runtime_error("Could not start transaction");

try
{
   // Execute commands that use this same connection.
   if (!connection.commitTransaction())
      throw std::runtime_error("Could not commit transaction");
}
catch (...)
{
   connection.rollbackTransaction();
   throw;
}
```

These methods return `false` when the connection handle is empty or when the
backend reports failure. The wrapper does not add nested-transaction or
cross-connection guarantees. Commands and tables created from this connection
use the same underlying connection, so their work is part of its active
transaction. A command from another acquired pool connection is a different
connection and is not part of that transaction.

## Lifetime and ownership

`DbConnection`, `DbCommand`, `DbResult`, and `DbTable` are move-only handles.
They cannot be copied. A command or table created from a connection does not
consume that connection. Internally, these objects retain shared connection
state, so a command, result, or table can keep the underlying connection alive
after the original `DbConnection` handle has been destroyed or moved from.

This also applies to pooled connections. `DbConnectionPool::acquire()` returns
a connection lease. The lease is not available for reuse while objects that
refer to its connection state are still alive. Destroy commands, results, and
tables when their work is done; do not assume that destroying only the local
connection handle immediately releases the pool slot. `close()` on a command
closes its command resources, but the command wrapper still exists and retains
its state until it is destroyed or replaced.

Create a pool with a nonzero maximum size, then acquire a connection for a
bounded unit of work:

```cpp
DbConnectionPool pool("main", options, 4);
{
   DbConnection connection = pool.acquire();
   auto command = connection.createCommand("SELECT 1");
   auto value = command.executeScalar();
}
```

The no-argument `acquire()` waits indefinitely, as before. To limit that wait,
pass `PoolAcquireOptions` with a `std::chrono` duration or a
`PoolAcquireCancellationToken`. The pool throws `PoolAcquireTimeout` or
`PoolAcquireCancelled` if the request stops while waiting for capacity. The
same options are available on `ConnectionPool::acquire()` and on the pooled
`DbServiceFactory::getConnector()` and `createService()` overloads. These
options do not interrupt a connection attempt after a slot has been reserved.
See the [connection-pool guide](connection_pool.md) for FIFO and cancellation
semantics.

A pool size of zero throws `std::invalid_argument`. An unavailable pool or
connection can throw. Pooling limits simultaneous connection leases; it does
not make operations on one connection safe for concurrent use. The `Db*` API
does not promise thread safety for a shared connection, command, result, or
table. Serialize access unless the selected backend and your usage provide a
stronger guarantee.

## Practical checks

- Use parameter binding for values. Do not concatenate untrusted values into
  SQL text. Identifiers such as table and column names are SQL syntax, not
  parameter values; validate or select them from trusted choices.
- Call `DbTable::isValid()` after `init()`, and check the result of `save()`
   and transaction calls.
- Check `DbResult::isValid()` and confirm a named column exists before reading
  it. A missing name produces column index `-1`.
- Treat `DbTable::deleteRows()` as an immediate database operation.
- Catch exceptions at a boundary that can add useful context. Do not include
  passwords or other credentials in error logs.
- Test SQL, type conversion, and editable-table behavior against each backend
  you support. The API is common; backend syntax, type mappings, and some
  operations remain backend-specific.
