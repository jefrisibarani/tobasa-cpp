# TobasaSQL connection pools

A connection pool keeps a limited number of database connections and lends
them to callers for short periods. Return a connection by releasing its RAII
handle. The pool can then lend that connection to another caller instead of
opening a new one.

For most code using the non-templated `Db*` API, use `DbConnectionPool`:

```cpp
#include <tobasasql/db_api.h>

int main()
{
   using namespace tbs::sql;

   DbConnectionOptions options;
   options.backend = BackendType::sqlite;
   options.connectionString = "Database=./app.db3;OpenCreate=True;";

   DbConnectionPool pool("main", options, 4);
   {
      DbConnection connection = pool.acquire();
      auto command = connection.createCommand("SELECT 1");
      std::string value = command.executeScalar();
   }
}
```

When the `connection` and its dependent objects leave scope, the lease returns
the connector to the pool. The next caller can reuse it.

## What the pool stores

`ConnectionPool` stores `DatabaseConnector` objects and hands them out via a
move-only RAII lease, `PooledConnection`. When that lease is destroyed, it
returns the connector to the pool and wakes a waiting caller.

The pool reuses idle connectors first. If none are idle and the pool has not
reached its maximum size, it creates a new connector. Otherwise, `acquire()`
waits until a slot or a connector becomes available.

The no-argument `acquire()` waits without a timeout. The overload with
`PoolAcquireOptions` supports timeout and cancellation. It also reserves queue
slots in FIFO order, so callers are admitted in the order they entered the wait
queue.

The maximum size is a hard cap on the number of connectors created by that
pool, not a target size. Returned connectors stay idle for reuse until they are
borrowed again; there is no idle timeout or automatic shrink policy.

## Choosing a pool API

### `DbConnectionPool`

`DbConnectionPool` is a small wrapper for the backend-neutral `Db*` API. It
takes a name, `DbConnectionOptions`, and a maximum size. The maximum defaults
to 4. `acquire()` returns a move-only `DbConnection`; create commands, results,
and tables from that handle as usual. Its option-taking `acquire()` overload
accepts the same `PoolAcquireOptions` as the lower-level pool.

This wrapper owns the underlying `ConnectionPool` through a shared pointer, so
callers do not need to construct or manage `ConnectionPool` directly. A size
of zero throws `std::invalid_argument` during construction.

### `ConnectionPool` and `PooledConnection`

The lower-level API accepts a `conf::ConnectorOption` and returns a
`PooledConnection`. Use it when code already works with
`DatabaseConnector` or needs the pool handle directly. Since
`ConnectionPool::acquire()` uses `shared_from_this()`, create the pool under
`std::shared_ptr` ownership, for example through `ConnectionPoolManager` or
`std::make_shared`. Calling `acquire()` on a stack-allocated `ConnectionPool`
throws `std::bad_weak_ptr`.

### `ConnectionPoolManager` and `DbServiceFactory`

`ConnectionPoolManager` keeps pools by name. Reusing the same name is only
allowed when the existing pool has the same settings and the same maximum size.
A different option set or size throws `std::invalid_argument`.

`DbServiceFactory` uses the manager for pooled connectors and services. Its
default pool size is 4, and that value is used as a fallback when a connector
option does not set its own pool size. For a bounded wait, pass
`PoolAcquireOptions` after the existing `pooled` argument to
`getConnector()` or `createService()`.

The manager protects its pool map with a mutex and the connection pool protects
its idle queue and connection count with its own mutex. That keeps pool
acquisition and release safe across threads. It does not make a borrowed
connection safe for concurrent use by multiple threads; treat one lease as one
unit of work.

## Timeout and cancellation

Use `PoolAcquireOptions::timeout` to set a wait limit. The timeout uses
`std::chrono` and is measured with `steady_clock`. A negative duration throws
`std::invalid_argument`.

Use a `PoolAcquireCancellationSource` when a waiting caller may be cancelled
from another thread. Tokens are copyable, and cancelling a source wakes the
matching waiters. It does not interrupt a connection creation or reconnect that
has already started.

The pool reports these cases as `PoolAcquireTimeoutError` and
`PoolAcquireCancelledError`. These errors end the wait, not the database
operation itself. Once a caller reserves a slot, connector creation or
reconnection continues through the normal connection error path.

## Lease lifetime

The lease controls when a connection goes back to the pool. Keep the
`PooledConnection` alive for the whole operation. With `DbConnectionPool`, the
returned `DbConnection` owns that lease internally.

A copied `shared_ptr<DatabaseConnector>` is not the same as a longer lease.
That pointer can stay valid, but it is not a guarantee that the connection is
still checked out. Do not keep or use it after the lease is released or
destroyed.

Commands, results, and tables can keep the underlying connection state alive,
but they do not extend the lease by themselves. The pool keeps the connection
reserved until the lease is destroyed, and then it is returned to the idle
queue.

## Errors and recovery

A failed connection attempt throws as soon as the pool needs a new connector.
The wrapper `DbConnectionPool::acquire()` can also throw if its pool handle is
missing or the acquired connector is not connected. SQL operations keep their
normal driver or SQL exceptions.

If a new connector cannot be created, the pool releases the reserved slot,
wakes a waiting caller, and rethrows the failure. If an idle connector is
disconnected, the pool tries to reconnect it before reuse. If that fails, it
removes the broken connector and lets the next acquisition create a replacement.

The pool recovers capacity, but it does not retry the failed operation for the
caller. The caller still gets the exception and decides whether to retry.

## Practical use

- Keep the lease scope short; do not hold a pool connection while waiting on
  user input or unrelated work.
- Use one pool per backend configuration and give it a stable name.
- Pick a reasonable maximum size for the database's connection limit and your
  concurrency needs.
- Handle acquisition errors separately from SQL errors.
- Commit or roll back explicitly; the pool also does a defensive rollback when
  returning a connector to the idle pool.
- Prefer `DbConnectionPool` for simple backend-neutral code. See the [`Db*` API
  guide](db_api.md) for command, result, and connection lifetime details.
