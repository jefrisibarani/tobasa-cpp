# Web Service Application Startup Flow

This flow follows `src/app_server/src/main.cpp` through
`tbs::web::Webapp::start()` and `Webapp::runHttpServer()`.

## Process Entry

1. `main()` creates `ModuleConfig` and disables page release mode.

2. Time-zone data is initialized. Startup stops with exit code `1` if this
   fails.

3. `main()` constructs `web::Webapp`.

4. The application loads `appsettings.json`, using the embedded configuration
   as a fallback. If loading fails, `main()` returns `1`.

5. The `webapp` configuration is read and applied to resource handling. The
   configured database connection, HTTP settings, TLS settings, thread counts,
   and application options become available to the startup sequence.

## Application Assembly

6. Optional database migrations are added to `webapp.migrationJob()`.

7. The application creates `DbServiceFactoryApp`, adds the main database
   connector option, and gives the service to `Webapp` with `useDbService()`.

8. The application creates `EventEngine`, configures the home page, and loads
   HTTP response-header rules.

9. The application registers the status-result content builder.

10. Middleware is registered in this order:

   1. Exception handling
   2. Database connectivity check
   3. Multipart support
   4. Response-header rules
   5. Request identification/CORS-related processing
   6. Cache-control
   7. Content-type validation
   8. Session
   9. Authentication
   10. Authorization

11. Controllers are registered, including the core, users, administration,
   API, and optional test controllers.

12. If enabled, the LIS module is initialized. Its start and stop functions are
   stored in `ModuleConfig` for later lifecycle callbacks.

13. The default TLS asset callback is registered.

14. `webapp.onStart()` is registered to start the LIS engine, and
   `webapp.onStop()` is registered to stop it.

## `Webapp::start()`

15. `Webapp::start()` verifies that configuration was loaded.

16. Old session files are removed.

17. `setThreadPoolSize()` calculates and stores:

   - IO thread count;
   - HTTP worker thread count;
   - database connection-pool size.

18. If no custom database service was supplied, `Webapp` creates the default
   database service. In this application, the custom `DbServiceFactoryApp`
   has already been supplied.

19. The database connection-pool size is applied and the database service is
   attached to the web service.

20. The migration job runs. Startup then checks database connectivity. If the
   database is unavailable, `start()` returns `1`; migration exceptions are
   logged and startup continues to the subsequent connectivity check.

21. `_pWebService->setupHandlers()` finalizes web-service routing and handler
   setup.

22. `runHttpServer()` is called.

## HTTP and HTTPS Server Construction

23. `runHttpServer()` creates the HTTP worker `asio::thread_pool`.

24. It builds `http::Settings` from the configured HTTP options, including:

   - address and ports;
   - read, write, and processing timeouts;
   - read, send, and header buffer sizes;
   - multipart and temporary-directory settings;
   - compression settings;
   - rate-limiter settings.

25. A `PlainServer` is constructed with the shared application IO context and
   receives the configured status-page builder.

26. A `SecureServer` is constructed with TLS certificates, TLS asset settings,
   HTTPS settings, and the same request-handler selection.

27. If worker threads are enabled, both servers route requests through
   `Webapp::handleRequest()`. Otherwise, both use the web service's direct
   HTTP request handler.

28. `WebappAgent` receives pointers to the plain and secure server objects and
   records their configured ports.

## Server Start Scheduling

29. A SIGINT handler is registered on the IO context. On SIGINT it posts a
   shutdown operation to the plain-server executor. That operation stops the
   configured servers and calls `Webapp::shutdown()`.

30. A separate task is posted to the plain-server executor to start the
   servers:

   - HTTPS-only mode starts only `secureServer`;
   - normal mode starts `plainServer` and `secureServer`.

31. After successful server startup, the task records `startedTime` in the
   `WebappAgent` status.

32. The posted server-start task is asynchronous. `callOnStartFunctor()` is
   called immediately after posting it, before the IO context necessarily
   executes that task. Therefore, the application's `onStart` callback is
   scheduled in startup order before the event loop begins processing the
   posted server-start operation; it should not assume the listening sockets
   have already started unless that is guaranteed by the executor state.

33. The IO context begins running:

   - with `ioPoolSize <= 0`, `run()` executes on the current thread;
   - with an IO pool, `runIoContextOnThreadPool()` starts the IO threads and
     joins them.

34. If worker threads are enabled, `Webapp::start()` joins the worker pool
   after the IO context stops.

## Shutdown Boundary

35. A SIGINT or another shutdown request stops the servers, stops the IO
   context, joins worker and IO threads, invokes the registered `onStop`
   callback, and runs final cleanup.

36. In this application, `onStop` invokes `moduleConfig.stopLisEngine()`.

```text
main()
  |
  +--> initialize timezone data
  +--> construct Webapp
  +--> load configuration
  +--> create DB service and EventEngine
  +--> register migrations, middleware, controllers, TLS assets
  +--> register onStart/onStop
  |
  v
Webapp::start()
  |
  +--> clear old sessions
  +--> calculate IO/worker/DB pool sizes
  +--> run migrations and connect database
  +--> setup web-service handlers
  |
  v
runHttpServer()
  |
  +--> create worker pool
  +--> construct PlainServer and SecureServer
  +--> configure request handlers and status
  +--> post server-start task
  +--> call onStart functor
  |
  v
IO context runs
  |
  +--> start plain/secure server task executes
  +--> accept HTTP/TLS connections
  +--> process requests
  |
  v
SIGINT or shutdown request
  |
  +--> stop servers
  +--> stop IO context
  +--> join pools
  +--> call onStop
  +--> cleanup
```
