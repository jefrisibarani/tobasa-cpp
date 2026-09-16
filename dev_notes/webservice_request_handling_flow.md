# HTTP Request Handling Flow (simplified)

This is the practical request path from browser to controller, based on the actual wiring in `tobasahttp`, `tobasaweb`, and `app_server`.

## 1. Browser request enters the server

1. The browser connects to the HTTP listener.
2. The socket is accepted by the HTTP server.
3. The listener creates a `ServerConnection` and hands it to the connection manager.
4. `ConnectionMgr::addConnection()` registers the connection, assigns an ID, and calls `conn->start()`.
5. `ServerConnection::start()` sets the parser state and calls `startHttp1()` (or HTTP/2 path when enabled).
6. `startHttp1()` begins reading data from the socket with `async_read_some()`.

At this stage, the raw TCP bytes are only being read and parsed. Nothing in the app layer has run yet.

## 2. HTTP parsing completes

When the parser has enough bytes to build a full request:

- `retrieveRequest()` creates the request/response `HttpContext`.
- headers, method, path, query, auth data, and body metadata are filled in.
- connection keep-alive settings are determined.
- if the request is an upgrade request, WebSocket/SSE handling takes over.
- otherwise, `handleRequest()` is called.

This is the key point where the app’s HTTP pipeline starts.

## 3. Request handler is the middleware chain

The confgured server request handler is not a direct controller call. It is a chained pipeline built by `WebService` and `MiddlewareManager`.

The actual wiring is:

- `Webapp::runHttpServer()` configures either:
  - direct `handleRequest(context)` when worker threads are enabled, or
  - `_pWebService->serverHttpRequestHandler()` when the service owns the request handler
- `WebService::serverHttpRequestHandler()` creates a `tbs::http::RequestHandler` that calls `MiddlewareManager::invoke()`
- `MiddlewareManager::doInvoke()` builds the middleware chain and then invokes the first middleware handler

That means the server is effectively doing:

```text
socket bytes -> HTTP parser -> HttpContext -> MiddlewareManager -> Router -> Controller
```

## 4. Middleware chain execution

`Webapp::start()` registers middleware in order in `app_server/src/main.cpp`:

1. Exception handler
2. Database connectivity check
3. Multipart support
4. Response header rules
5. Request identification
6. Cache control
7. Content type validation
8. Session
9. Authentication
10. Authorization

Each middleware is wrapped as a `Middleware` object and added to the service via `WebService::addMiddleware()`.

`MiddlewareManager::doInvoke()` configures the chain by setting each middleware's `nextHandler()` to the following middleware's handler. The first middleware then calls `next(context)` when it wants to continue the pipeline.

This is classic middleware chaining:

```text
Exception -> DBCheck -> Multipart -> HeaderRule -> RequestID -> Cache -> ContentType -> Session -> Auth -> Authz -> Router -> Controller
```

If a middleware handles the request itself, it may return `handled` without calling `next(context)`.

## 5. Router resolves the route and picks the controller

The final stage before application logic is the router.

When `MiddlewareManager` runs the chain, it calls `Router::setupRoute(context)` before invoking the first handler. `setupRoute()` loops through registered route entries, compares the request method and path, and picks the matching route.

Then:

- `context->requestHandlerId(entry.id())` records which route matched
- `context->request()->authContext()->definedScheme` is assigned from the route auth policy
- authentication rules are resolved from config and effective scheme selection

When the router executes its matched handler:

- `RouteEntry::invokeHandler()` calls the registered handler
- the handler typically creates a `Result` object (HTML, JSON, redirect, status, etc.)
- the router serializes the result with `result->toResponse(response)`

This is the actual controller hook point.

## 6. How a controller is registered

In `app_server/src/main.cpp`, controllers are added like this:

- `CoreController`
- `ApiUsersController`
- `AdminController`
- `ApiCoreController`
- optional test controllers

Each controller derives from `web::ControllerBase` and implements `bindHandler()`. The factory then links the registered route functions into the router.

In other words, the controller is not called directly by the server. It is registered with the router, and the router calls it when the request matches its path and method.

## 7. The simplified flow

```text
Browser
  |
  v
HTTP listener accepts socket
  |
  v
ServerConnection::start()
  |
  v
HTTP parser reads bytes / headers / body
  |
  v
request complete -> ServerConnection::handleRequest()
  |
  v
_requestHandler = MiddlewareManager::invoke()
  |
  +--> middleware chain runs
  |       (Exception, DB, ContentType, Session, Auth, ...)
  |
  v
Router::setupRoute() selects matching route
  |
  v
Controller handler bound in bindHandler() runs
  |
  v
Result is produced and serialized to response
  |
  v
write() sends HTTP response back to browser
```

## 8. Response path

After the controller prepares the result:

- the router serializes it into `context->response()`
- `ServerConnection::write()` prepares compression if needed
- `asio::async_write()` sends the bytes to the client
- the connection either stays alive for another request or closes according to HTTP keep-alive rules

For SSE and WebSocket there are upgrade paths, but normal HTTP request handling remains the middleware -> router -> controller sequence above.

## 9. Practical Model

- `tobasahttp` owns transport parsing and HTTP message lifecycle
- `tobasaweb` owns middleware + routing + controller registration
- `app_server` assembles the actual app by adding business middleware and controller handlers

The browser calls the server socket; the server parses HTTP; the middleware chain runs; the router selects a controller; the controller returns a result; then the response is written back.

HTTP parsing happens in the transport layer, middleware decides request processing, the router matches the URL/method to a controller, and the controller produces the HTTP result that gets written back to the client.
