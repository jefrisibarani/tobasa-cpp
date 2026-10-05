# Web Service Application Startup and Shutdown Flow

This note follows the current code in `src/app_server/src/main.cpp` and `src/tobasaweb/src/tobasaweb/webapp.cpp`.

## 1. Startup entry

1. `main()` creates a `ModuleConfig` object to keep the LIS start and stop callbacks.
2. `web::Page::releaseMode(false)` keeps the app in development mode.
3. `DateTime::initTimezoneData()` runs first. If it fails, the app exits with code `1`.
4. A `web::Webapp` object is created.
5. The app loads `appsettings.json` from the app folder and uses embedded config as a fallback when needed.
6. `webapp.loadConfig()` loads the app settings and sets up the logger with `log::MultiLogger`.
7. If the temp folder or TLS files are not valid, startup stops with an error.

## 2. Building the application in `main()`

8. The app reads the `webapp` settings and sets the in-memory resource flag for the current build.
9. It adds database migrations to `webapp.migrationJob()`:
   - test migration when `TOBASA_USE_TESTS_MODULE` is enabled
   - LIS migration when `TOBASA_USE_LIS_ENGINE` is enabled
10. A custom `DbServiceFactoryApp` is created, configured with the main database connection, and passed into the app with `webapp.useDbService(dbService)`.
11. An `EventEngine` is created and passed to the app controllers.
12. The home page is set with `tbs::web::Page::homePage()`.
13. HTTP response-header rules are loaded from the app config.
14. A custom status result builder is registered for the router.
15. Middleware is added in this order:
   - exception handler
   - database connectivity check
   - multipart parsing
   - response-header rules
   - request identification
   - cache-control
   - content type validation
   - session handling
   - authentication
   - authorization
16. Controllers are added for the main app, user API, admin API, and application APIs.
17. If the LIS engine is enabled, `AppLisModule` is created and its start/stop callbacks are stored in `ModuleConfig`.
18. The default TLS asset callback is registered so HTTPS resources can be served from embedded data.
19. `webapp.onStart()` is set to call `moduleConfig.startLisEngine()`, and `webapp.onStop()` is set to call `moduleConfig.stopLisEngine()`.

## 3. `Webapp::start()`

20. `Webapp::start()` runs after the app is assembled.
21. It checks whether config is valid. If not, it returns `1`.
22. `Session::clearOldSessionFiles()` removes old session files.
23. `setThreadPoolSize(ioPoolSize, workerPoolSize)` calculates:
   - IO thread count
   - HTTP worker thread count
   - database connection pool size
24. If no custom database service is supplied, the app creates one. In this app, a custom DB service is already supplied, so that path is skipped.
25. The DB service is set with the computed pool size and attached to the web service with `_pWebService->useDbService(_dbService)`.
26. The migration job runs, and the app checks if the database is connected. If not, startup stops with `1`.
27. `_pWebService->setupHandlers()` finishes the routing and middleware setup.
28. `runHttpServer()` is called to start the HTTP and HTTPS listeners.

## 4. Starting the HTTP and HTTPS servers

29. `runHttpServer()` creates the worker pool used for request handling.
30. It builds the `http::Settings` and `http::SettingsTls` objects from the app config, including:
   - listen address and ports
   - read/write/processing timeouts
   - buffer sizes
   - multipart and temp folder settings
   - compression settings
   - rate limiting settings
   - TLS certificate files and callbacks
31. A `PlainServer` and a `SecureServer` are created using the same `asio::io_context`.
32. The request handlers are assigned to `Webapp::handleRequest()` when worker threads are enabled. Otherwise, the direct web service handler is used.
33. `WebappAgent` stores the server pointers and the configured ports.

## 5. Startup timing and callback order

34. `runHttpServer()` installs a `SIGINT` handler on the IO context.
35. When Ctrl+C is pressed, the handler does not stop the app right away. Instead, it posts a task to the server executor that:
   - stops the HTTP servers
   - calls `shutdown()`
36. Another task is posted to start the server:
   - HTTPS-only mode starts only `secureServer`
   - normal mode starts both `plainServer` and `secureServer`
37. When the startup task succeeds, it sets `_myAgent->_status.startedTime`.
38. `callOnStartFunctor()` is called right after the startup task is posted, not after the server has definitely started listening. This means the app's `onStart` callback runs in the normal startup sequence, but before the event loop has necessarily processed the start task.
39. If `_ioPoolSize <= 0`, the IO context runs on the current thread. Otherwise, it starts IO threads with `runIoContextOnThreadPool()`.
40. After the IO context stops, `Webapp::start()` joins the worker pool if one was created.

## 6. The actual shutdown process

41. Shutdown can start from the signal handler or by calling `Webapp::shutdown()` directly.
42. `Webapp::shutdown()` is protected by a `std::atomic<bool>`, so only the first call is allowed through:

```cpp
bool expected = false;
if (!_shutdownCalled.compare_exchange_strong(expected, true))
   return;

_ioContext.stop();
```

43. So shutdown is safe to call multiple times. Extra calls are ignored.
44. The actual web servers are stopped before `shutdown()` is called in the posted SIGINT task:

```cpp
if (_appOption.httpServer.runHttpsOnly)
   secureServer.stop();
else
{
   plainServer.stop();
   secureServer.stop();
}

shutdown();
```

45. Calling `_ioContext.stop()` stops the event loop and ends the IO work.
46. After the IO context exits, `Webapp::start()` does this:
   - joins the worker pool if it exists
   - calls `callOnStopFunctor()`
   - calls `cleanup()`
47. `callOnStopFunctor()` runs the app-level `onStop` callback. In this app, that callback calls `moduleConfig.stopLisEngine()`, which is how the LIS engine is shut down.
48. Final cleanup resets the DB service pointer, clears the logger sink, and marks the app as stopped.
49. The destructor also calls `cleanup()`, so the object remains safe even if shutdown happens during destruction.

## 7. Lifecycle summary

```text
main()
  +--> initialize timezone data
  +--> create Webapp
  +--> load config
  +--> add migrations
  +--> create DB service and event engine
  +--> register middleware and controllers
  +--> register onStart/onStop
  +--> start webapp

Webapp::start()
  +--> validate config
  +--> clear old sessions
  +--> calculate thread sizes
  +--> configure DB service
  +--> run migrations and check DB
  +--> set up handlers
  +--> start HTTP server

runHttpServer()
  +--> build plain and secure servers
  +--> attach request handlers
  +--> post startup task
  +--> call app onStart callback
  +--> run IO context

Shutdown
  +--> Ctrl+C triggers stop task
  +--> stop plain/secure servers
  +--> stop IO context
  +--> event loop exits
  +--> join worker pool
  +--> run app onStop callback
  +--> clean app resources
```

## 8. Important shutdown note

The current shutdown flow is not a big multi-step routine inside `Webapp::shutdown()` itself. The real sequence is:

1. stop the active HTTP servers,
2. tell the IO context to stop,
3. let the loop exit,
4. join the worker threads,
5. run the registered `onStop` callback,
6. clean up resources.
