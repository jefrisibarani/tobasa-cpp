
# Tobasa Web Library

A production-ready C++ web framework for building REST APIs and server-side applications with routing, middleware, and authentication.

## Overview

Tobasa Web provides comprehensive server-side application infrastructure built on top of the Tobasa ecosystem. It combines HTTP transport (`tobasahttp`), database abstraction (`tobasasql`), and authentication/session management into a cohesive framework for building scalable REST APIs and web services.

The library handles all infrastructure concerns—routing, middleware pipelines, request/response handling, authentication, and database integration—so you can focus on business logic in controllers and services.

## Features

**Routing & Controllers:**
- RESTful routing with path parameters and HTTP method matching
- Controller-based request handlers
- Pattern matching for flexible route definitions
- Per-route authentication and authorization rules

**Middleware Pipeline:**
- 10-layer middleware chain (exception handling, authentication, session management, etc.)
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
| **Middleware** | Request processing pipeline (10-layer chain) |

## Configuration

Configure your application in `appsettings.json`:


## Middleware Pipeline

The 10-layer middleware chain processes each request:

1. **Exception Handler** – Catches unhandled exceptions
2. **Database Check** – Validates database connectivity
3. **Multipart Parser** – Parses file uploads
4. **Response Headers** – Applies CORS and security headers
5. **Request Identification** – Tracks request ID and client info
6. **Cache Control** – Manages HTTP caching
7. **Content-Type Validation** – Validates request content
8. **Session Management** – Loads/maintains sessions
9. **Authentication** – Validates JWT or session
10. **Authorization** – Enforces access control (ACL)


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
- Efficient request routing (trie-based)
- Connection pooling reduces latency
- Middleware pipeline optimized for throughput
- Suitable for high-concurrency scenarios

## License

GNU LESSER GENERAL PUBLIC LICENSE