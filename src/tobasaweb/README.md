
# Tobasa Web Library

Tobasa Web provides routing, middleware, controller, authentication, and
session APIs on top of `tobasahttp` and `tobasasql`. The app server is an
example application built with these libraries; it is not part of the
`tobasaweb` library itself.

Applications compose the request pipeline by registering middleware and
controllers. Tobasa Web does not impose one fixed middleware count or order.

## Features

**Routing & Controllers:**
- RESTful routing with path parameters and HTTP method matching
- Controller-based request handlers
- Pattern matching for flexible route definitions
- Per-route authentication and authorization rules

**Middleware Pipeline:**
- Ordered middleware chain, including built-in session, CSRF, authentication, authorization, and multipart helpers
- Pluggable middleware components
- Request/response interception and transformation
- Error handling and logging integration

**Authentication & Security:**
- JWT token validation and generation
- Session management with configurable storage
- Cookie handling and secure defaults
- Per-route and per-controller access control (ACL)
- Role-based authorization

**Database Integration:**
- Seamless `tobasasql` integration
- Connection pooling and transaction support
- Automatic database initialization
- Migration support

**HTTP Features:**
- Full HTTP/1.1 support via `tobasahttp`
- HTTP/2 support (optional)
- TLS/HTTPS with certificate management
- WebSocket support
- SSE support
- Respond compression
- Multipart form data and file uploads
- Content negotiation

## Building

The library is built as part of the main build system:

## Core Components

| Component | Purpose |
|-----------|---------|
| **Webapp** | Application builder and lifecycle management |
| **Router** | HTTP route definition and matching |
| **ControllerBase** | Base class for request handlers |
| **ControllerFactory** | Dependency injection and controller creation |
| **Middleware** | Ordered request-processing chain |

## Middleware and security

`Webapp::useSession()`, `useCsrf()`, `useAuthentication()`, and
`useAuthorization()` add the corresponding built-in middleware. Register
session handling before CSRF and authentication. The application chooses its
middleware order and route policy; see
[built-in-middlewares.md](doc/built-in-middlewares.md).

Sessions use the filesystem backend by default and can use a plugin-backed
store. The app-server build includes a SQLite session-store plugin in shared
library mode. The filesystem backend does not imply that session cookies or
session data are automatically secure; review cookie and storage settings for
the deployment.


## Architecture

**Layered Design:**
- **Transport Layer** – `tobasahttp` handles HTTP/TLS
- **Framework Layer** – `tobasaweb` provides routing, middleware, controllers
- **Data Layer** – `tobasasql` abstracts database operations
- **Application Layer** – Your business logic in controllers and services

**Separation of Concerns:**
- Infrastructure (routing, middleware) handled by the framework
- Business logic isolated in controllers and service classes
- Database queries in repository classes
- Views/templates outside the scope (API-focused)

## Dependencies

- **tobasa** – Core framework
- **tobasahttp** – HTTP transport layer
- **tobasasql** – Database abstraction
- **asio** – Async I/O
- **nlohmann/json** – JSON handling
- **spdlog** – Logging
- **jwtcpp** – JWT token handling
- **OpenSSL** – TLS/HTTPS support

## Use Cases

- RESTful API backends
- Microservices
- Web service applications
- Real-time applications (with WebSocket)
- Backend integration layers

## Performance Characteristics

- Non-blocking async I/O
- Route matching checks registered routes in order.
- Database connection pooling is provided by TobasaSQL, not by the router.

## License

GNU LESSER GENERAL PUBLIC LICENSE