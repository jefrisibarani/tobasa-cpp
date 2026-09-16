# Using the Tobasa application server

This document is for someone who wants to run or extend the Tobasa web
service. It explains the useful parts of the server in simple terms:
configuration, startup, middleware, controllers, routes, and HTTPS.

For the complete list of routes, see [`endpoints.md`](endpoints.md). For all
configuration fields, see [`configuration_reference.md`](configuration_reference.md).

## What the application server does

The application server is a ready-to-use HTTP service built with Tobasa. It
can provide:

- normal HTTP and HTTPS requests;
- JSON APIs and HTML pages;
- cookie and bearer-token authentication;
- sessions and authorization checks;
- file and embedded web resources;
- multipart uploads;
- WebSocket and server-sent event connections;
- optional database, LIS, and test modules.

You normally extend it by adding a controller, registering routes, and adding
middleware when a request needs special processing.

## Startup in practical terms

The application starts in `src/app_server/src/main.cpp`. The important steps
are:

1. Create a `web::Webapp` object.
2. Load `appsettings.json`. If the file is missing, the embedded configuration
   is used.
3. Read the web server and database settings.
4. Create the database service and register database migrations.
5. Add application middleware.
6. Add controllers and their routes.
7. Add optional modules such as LIS or the test module.
8. Configure the default TLS assets.
9. Call `webapp.start()`.

The application does not need a separate route-registration step after
`start()`. `WebService::setupHandlers()` connects the middleware and
controller factories to the router before requests are accepted.

## The request path

For a normal request, the flow is:

```text
client
  -> HTTP or HTTPS listener
  -> HTTP parsing
  -> web middleware
  -> authentication and authorization
  -> router
  -> controller handler
  -> http::Result
  -> http::Response
  -> client
```

The controller usually returns an `http::Result`. The web layer converts that
result into an `http::Response`, which is then serialized and sent by the
HTTP server.

The exact middleware order depends on how the application adds middleware.
In the current application, the chain includes exception handling, database
checking, multipart parsing, response-header rules, request identification,
cache control, content-type checks, sessions, authentication, and
authorization.

## Adding a controller

A controller normally inherits from `web::ControllerBase` and registers its
routes in `bindHandler()`:

```cpp
class HealthController : public web::ControllerBase
{
public:
   void bindHandler() override
   {
      router()->httpGet(
         "/health",
         std::bind(&HealthController::onHealth, this,
                   std::placeholders::_1));
   }

   http::ResultPtr onHealth(const web::RouteArgument& arg)
   {
      return http::makeResult<http::Result>("OK", "text/plain");
   }
};
```

Then add the controller to the web application before calling `start()`:

```cpp
webapp.addController(
   web::makeController<HealthController>());
```

Use the router method that matches the HTTP method:

```cpp
router()->httpGet("/items", handler);
router()->httpPost("/items", handler);
router()->httpPut("/items", handler);
router()->httpDelete("/items", handler);
```

Typed path values are declared in the route and read from `RouteArgument`:

```cpp
router()->httpGet(
   "/users/{user_id:int}",
   std::bind(&UserController::onUser, this,
             std::placeholders::_1));
```

The application route API currently registers `GET`, `POST`, `PUT`, and
`DELETE` handlers. The lower HTTP parser also recognizes `HEAD` and
`OPTIONS`, but `app_server` does not register application handlers for them.

## Returning a result

For controller code, returning a result is usually simpler than constructing
a response by hand:

```cpp
http::ResultPtr UserController::onUser(const web::RouteArgument& arg)
{
   auto userId = arg.get("user_id");
   if (!userId)
      return web::badParameter("user_id is required");

   auto user = findUser(userId.value());
   if (!user)
      return web::notFound("User not found");

   auto result = http::makeResult<http::Result>(user->json().dump(),
                                                "application/json");
   return result;
}
```

When using the application server, common result helpers include:

- `web::object(value)` for JSON data;
- `web::okResult(message)` for a successful message;
- `web::badRequest(message)` for an HTTP 400 response;
- `web::badParameter(message)` for an application validation error;
- `web::notFound(message)` for an HTTP 404 response;
- `web::unauthorized(message)` and `web::forbidden(message)` for access errors;
- `web::appError(message)` for an HTTP 500 response.

The application-specific helpers are implemented in
[`src/app_server/src/api_result.h`](../src/api_result.h). A library user can
also create a class derived from `http::Result` when a different response
format is needed.

## Middleware

Middleware receives the request context and a `next` handler. It can inspect
the request, stop the request with its own result, or call `next` and continue
the chain.

```cpp
webapp.addMiddleware(
   [](const http::HttpContext& context,
      const http::RequestHandler& next) {
      // Check or change the request here.
      return next(context);
   },
   "ExampleMiddleware");
```

Add middleware before adding controllers and before `webapp.start()`. Put
checks that must happen before routing early in the chain. Authentication and
authorization should stay in the web-service middleware layer instead of
being copied into every controller.

## Authentication

When a route is registered, it can declare an authentication scheme:

```cpp
router()->httpGet("/profile", handler, web::AuthScheme::COOKIE);
router()->httpGet("/api/data", handler, web::AuthScheme::BEARER);
router()->httpGet("/public", handler, web::AuthScheme::NONE);
```

The available schemes are `NONE`, `COOKIE`, `BEARER`, and `BASIC`.

This declaration is the route default. The `routeAuthLists` configuration is
also checked by the router and can change the effective rule for a matching
path. Check the configuration as well as the controller when debugging an
authentication problem.

## HTTP and HTTPS settings

The listener settings are under `webapp.httpServer`:

| Setting | Use |
| --- | --- |
| `address` | Address to bind. |
| `port` | Plain HTTP port. |
| `portHttps` | HTTPS port. |
| `runHttpsOnly` | Start only HTTPS when `true`. |
| `http2Enabled` | Enable HTTP/2 on HTTPS when it is compiled in. |
| `timeoutRead`, `timeoutWrite`, `timeoutProcessing` | Time limits for request work. |
| `maxHeaderSize` | Maximum request header size. |
| `maxRequestsPerConnection` | Maximum requests on one connection. |
| `ioPoolSize`, `workerPoolSize` | I/O and request worker counts. |
| `docRoot` | Directory used for files served from disk. |
| `temporaryDir` | Temporary directory for multipart uploads. |
| `enableMultipartParsing` | Enable multipart request parsing. |
| `compression` | Response compression settings. |
| `useRateLimiter` | Enable request rate limiting. |

With the default mode, the server starts both listeners. With
`runHttpsOnly: true`, it starts only the HTTPS listener. Both listeners use
the same middleware, router, and controller code.

## TLS setup

The default TLS settings are under `webapp.httpServer.tls`:

```json
"tls": {
   "certificateChainFile": "./tls_asset/server.crt",
   "privateKeyFile": "./tls_asset/server.key",
   "tmpDhFile": "./tls_asset/dh2048.pem"
}
```

Use certificates that match the hostnames used by clients. The application
has embedded development assets for the certificate, private key, and DH
parameters. They are useful for local testing, but they are not deployment
certificates.

For several hostnames, add entries to `hostCertificates`:

```json
"hostCertificates": [
   {
      "hostname": "api.example.com",
      "certificateChainFile": "./tls_asset/api.crt",
      "privateKeyFile": "./tls_asset/api.key",
      "password": ""
   }
]
```

The TLS SNI callback selects a host certificate when the client hostname
matches an entry. Otherwise, the default TLS context is used. Relative paths
are resolved relative to the executable by the application configuration
loader.

TLS contexts are created during startup. Changing certificate files or TLS
settings does not reload them in a running process; restart the application
after changing them.

## Files and embedded resources

The application can serve resources from the document root. A build can also
include web resources inside the executable. The setting
`webapp.webService.useInMemoryResources` controls whether the application uses
those embedded resources when that build feature is available.

The application also has embedded fallback configuration and TLS assets. This
does not mean that every file is embedded, so production deployments should
still provide their configured files and templates.

## WebSockets, SSE, and optional modules

WebSocket endpoints are registered as `GET` routes. The HTTP handler checks
the upgrade request and changes the connection to WebSocket processing. SSE
uses a long-lived HTTP response instead of a WebSocket upgrade.

The LIS and test controllers are optional. Their routes exist only when the
corresponding module is compiled and registered. Do not assume that routes in
[`endpoints.md`](endpoints.md) exist in every build.

## Practical troubleshooting

- A route returns 404: check the HTTP method, the exact path, and whether the
  controller was added to the application.
- Authentication fails: check both the route declaration and
  `routeAuthLists` in the configuration.
- A JSON controller rejects a request: check the request content type and
  body format. Many application handlers require `application/json`.
- Uploaded files are missing: check multipart parsing and `temporaryDir`.
- HTTPS does not start: check the certificate, key, DH file, and their paths.
  The startup log contains the configuration or TLS error.
- A route works in one executable but not another: check compile-time modules
  such as LIS and the test module.

## Shutdown

The application handles `SIGINT`, stops its listeners, stops optional modules,
and joins its I/O and worker threads. Startup or TLS configuration errors are
reported and prevent normal server operation.
