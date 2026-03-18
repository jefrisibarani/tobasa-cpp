
# Tobasa Web Library
C++ library for building Web applications and REST APIs using the Tobasa ecosystem.

## Overview
Tobasa Web is a server-side C++ library for building HTTP services such as Web APIs or backend applications.

It builds on top of tobasahttp for HTTP transport and tobasasql for database access, and adds higher-level web application features such as routing, controllers, middleware, session handling, and authentication helpers.

The library focuses on backend infrastructure (request handling, routing, authentication, etc.).
Frontend rendering or templating is intentionally outside its scope.

## Features
- HTTP routing and controller-based request handling
- Middleware pipeline for request processing
- Integration with `tobasahttp` (HTTP transport, TLS)
- Integration with `tobasasql` (database access and initialization)
- Built-in support for JWT authentication, session management, and logging

## Core Components
- `Webapp` — application builder and runtime (load config, start servers)
- `Router` and `RouteEntry` — define HTTP routes and handlers
- `ControllerBase` / `ControllerFactory` — create request handlers grouped by controller
- Middleware (authentication, session, multipart) — pluggable request processing

## Dependencies
- tobasa — core framework utilities
- tobasasql — database abstraction and helpers
- tobasahttp — HTTP transport layer
- spdlog — logging
- jwtcpp — JSON Web Token handling


## Configuration
Settings live in `appsettings.json` (see `conf::Webapp` / `conf::WebService` in
`settings_webapp.h`). You can configure:

- HTTP server options (ports, TLS settings)
- DB connection settings and pool size
- Session lifetime and storage path
- JWT issuer/secret and token timeouts
- Route authentication rules and sessions

## Design Notes
The library handles infrastructure concerns such as routing, middleware, and HTTP protocol handling.
Application-specific logic should be implemented inside controllers or separate service classes.


## Where to look in the code
- `include/tobasaweb/webapp.h` — app builder and runtime
- `include/tobasaweb/router.h` — routing and route matching
- `include/tobasaweb/controller_base.h` — controller base class
- `include/tobasaweb/*_middleware.h` — built-in middleware

## License
GNU LESSER GENERAL PUBLIC LICENSE