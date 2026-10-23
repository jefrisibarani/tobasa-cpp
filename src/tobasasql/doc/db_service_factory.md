# `DbServiceFactory`

Use `DbServiceFactory` when the app chooses its database config at runtime and then creates repo or service objects from that config.

The factory is small, but it sits in the middle of a few important pieces: connector config, connection pooling, and service lifetime. In practice, the app registers named database options once, then asks for a service by name.

The usual pattern in this codebase is:

1. register one or more connector configs,
2. set the default pool size if needed,
3. ask the factory for a repo or service,
4. let the factory keep the connection alive for the service lifetime when pooling is enabled.

## Example

```cpp
#include <tobasasql/database_service_factory.h>

using namespace tbs::sql;

conf::ConnectorOption options;
options.production.dbDriver = BackendType::sqlite;
options.production.connectionString = "Database=./app.db3;OpenCreate=True;";

DbServiceFactory factory;
factory.setConnectionPoolSize(8);
factory.addConnectorOption("MainDb", options, 4);

auto repo = factory.createService<MyRepo>("MainDb", true);
```

This is the basic flow. Register a config under a name, then create a service from that name.

If the connector has its own pool size, the factory uses it. If the pool size is `0`, it falls back to the global default set by `setConnectionPoolSize()`.

## What the factory is doing

`DbServiceFactory` is not a database abstraction in the ORM sense. It is a registry and factory for configured database connections.

In this project, it is mostly useful for:

- keeping named connector options,
- managing the default pool size,
- creating pooled or non-pooled database connectors,
- creating service objects from those connectors,
- wiring app-level database access to a main application DB plus named extras.

The implementation is simple at the top level. The real logic sits in the pool and in the app-specific subclass that maps names like `"AppDbRepo"` or `"PacsDbService"` to real repository types.

## Registering connector options

```cpp
factory.addConnectorOption("MainDb", options, 4);
```

`DbServiceFactory::addConnectorOption()` is a concrete method. It stores the
connector config in a map keyed by name and replaces an existing entry with
the same name. The pool size is per connector. A value of `0` means “use the
global default pool size.” A positive value selects a pool size for this
connector.

## Default pool size

The constructor starts with a default pool size of `4`:

```cpp
DbServiceFactory::DbServiceFactory()
   : _connPoolSize(4)
```

Then `setConnectionPoolSize()` changes that default for connectors that do not define their own size.

This is a global default, not a global cap. If a connector has `poolSize > 0`, it keeps its own pool size.

## Reading a connector config back

```cpp
conf::ConnectorOption getConnectorOption(const std::string& name, bool& isValid);
```

This returns the stored config and sets `isValid` to tell you whether the name existed.

If the name is missing, the function returns an empty `conf::ConnectorOption` and `isValid` is `false`.

## Creating a service

The factory provides a templated `createService()` overload for service
templates that accept the selected driver's connection type:

```cpp
template <template <class> class ServiceTemplate>
SqlServicePtr createService(const std::string& optionName, bool pooled = true);

template <template <class> class ServiceTemplate>
SqlServicePtr createService(const std::string& optionName, bool pooled,
                            const PoolAcquireOptions& acquireOptions);
```

For example, the sample's `sample::UserService` is a class template over the
driver connection type:

```cpp
auto service = factory.createService<sample::UserService>("MainDb", false);
```

This resolves the connector config, acquires or reuses a connection, then
creates the service specialization for the selected backend.

If `pooled` is `true`, it uses the pool manager and returns a pooled connection lease. If `pooled` is `false`, it creates a direct connector and keeps it in the non-pooled list.

An application-specific factory can wrap this pattern and map service names
to concrete service templates:

```cpp
sql::SqlServicePtr DbServiceFactoryApp::doGetDbService(
   const std::string& serviceName,
   bool pooled)
{
   if (serviceName == "AppDbRepo")
      return createAppDbRepo(pooled);
   else if (serviceName == "AuthDbRepo")
      return createAuthDbRepo(pooled);
   ...
}
```

So the generic factory is the infrastructure layer, and the app subclass decides which string maps to which repository or service type.

## The main app database is treated specially

A few methods are hard-wired to `MainAppDbOption`:

- `connect()`
- `connected()`
- `sqlConnPtrVariant()`
- `getSqlDriverVariant()`
- `testConnection()`

This factory is designed around a primary application database, with other named configs layered on top. It is not a fully abstract “any DB for any call” system. The design is “default app database plus named extra configs.”

## Pooled connections

This is the part that matters most in real use.

A pooled connection is managed by `PooledConnection`, which follows RAII. When the lease is destroyed, it releases the connection back to the pool. The code is designed so that the service keeps the lease alive while it is in use, and the connection is returned only after the service is gone.

The important detail is lifetime. The returned connector pointer is not the full ownership story. The actual pool ownership is tied to the lease object, and that lease is kept alive by the service lifetime. That is what makes pooled reuse safe and predictable.

This is a useful pattern, but it is easy to misunderstand if you think of the connector as a normal owning pointer. The raw pointer is valid for the service's lifetime, but the real ownership still sits in the pool lease.

## Service lifetime and pool lifetime

For pooled services, the service and the pool lease are kept together. The service object stays alive as long as the repo object is alive, and the pool lease stays alive as long as the service does.

This is important because the connection is not truly owned by the repo pointer alone. The real ownership sits in the pool lease, and the service keeps that lease alive for as long as it is in use.

In practice, this means the connection stays checked out while the service is alive, and it goes back to the pool when the service is destroyed. That is the lifetime boundary you want in a pooled service layer: no stale connection, no early return to the pool, and no accidental disconnect while the repo is still being used.

## Non-pooled connections

The direct connection path caches connectors in `_connectorList`.

If the connector name is already present, the factory reuses it. If it is not present, it creates a new `DatabaseConnector`, initializes the driver, connects it, and stores it.

If it exists but is disconnected, it tries to reconnect.

This gives you a long-lived direct connection for that connector name.

The tradeoff is simple: pooled connections are shared and recycled; non-pooled connections are owned by the factory for their lifetime.

## `disconnect()` behavior

`disconnect()` is a shutdown step, not a subtle “close only if needed” operation.

It does two main things:

1. it disconnects all non-pooled connectors in `_connectorList`,
2. it clears all connection pools with `ConnectionPoolManager::clear()`.

This is a blunt reset. It tears down the direct connectors and drops the pool manager state.

The actual pool cleanup logic is in `ConnectionPool::release()`, which calls `resetTransactionForPool()`. That method rolls back any active transaction before the connection is returned to the pool. If the connection is already broken, it disconnects it.

This is a good guarantee for pooled code: you do not want one user to leave a transaction open and leak it into the next user.

## Errors and edge cases

### Missing connector name

If a connector name is not registered, `getConnector()` throws
`std::runtime_error` for pooled access or `AppException` for non-pooled
access. Pooled `createService()` reports an invalid option with
`AppException("Invalid connector option: ...")`.

So a missing config is not treated like a silent null result.

### Duplicate pool key

The pool manager does not allow the same pool name to be reused with different settings.

If you create a pool under a name that already exists but with a different option set or a different pool size, it throws an error. This is a strict guardrail: pool names are meant to be stable and unique.

### Pool wait timeout and cancellation

`ConnectionPool::acquire(const PoolAcquireOptions&)` supports timeout and cancellation. These are not SQL problems; they are pool-wait problems.

The pool can throw:

- `PoolAcquireTimeoutError`
- `PoolAcquireCancelledError`

That is important in a server or background worker process where a blocking acquire would otherwise stall work for too long.

### Connect failure

If a connector cannot connect, the factory throws an app-level exception. Common cases include:

- `Failed to connect to database with option ...`
- `Failed to connect database for ...`
- `Database connection unavailable for ...`

The current code treats failed database access as a real error, not as a soft “false.”

## A few implementation details that matter

### SQLite gets extra setup on connect

After a successful SQLite connection, the code enables a few PRAGMAs:

- `foreign_keys = ON`
- `busy_timeout = 6000`
- `journal_mode = WAL`

This is not a public factory contract, but it is part of the real runtime behavior. It is mainly there to improve reliability and reduce lock-related surprises.

### The factory is name-based, not object-based

The whole design is keyed by string names. The config map, the pool key, and the service lookup are all name-based.

That is convenient, but it also means the names need to be stable and unique. If you reuse the same name with different settings, the behavior is not “smart”; it is explicit and strict.

