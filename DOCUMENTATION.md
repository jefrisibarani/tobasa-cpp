# Documentation index

This page lists the main documentation in this repository. The documents are
grouped by their purpose so it is easier to find a starting point.

## Development notes

These notes describe internal request, startup, and connection-lifecycle
flows. They are useful when changing the HTTP or WebSocket implementation.

| Document | What it covers |
| --- | --- |
| [`http_connection_cleanup_flow.md`](dev_notes/http_connection_cleanup_flow.md) | How an HTTP connection is removed after the browser closes it or the socket reports an error. |
| [`http_response_and_result_class.md`](dev_notes/http_response_and_result_class.md) | When to use `http::Response` and when a controller should return `http::Result`. |
| [`http_server_request_handling_flow.md`](dev_notes/http_server_request_handling_flow.md) | Low-level HTTP/1.x parsing, connection registration, and request processing. |
| [`sse_connection_cleanup_flow.md`](dev_notes/sse_connection_cleanup_flow.md) | How an SSE connection is removed after the client disconnects. |
| [`webservice_app_start_flow.md`](dev_notes/webservice_app_start_flow.md) | How the application server is assembled and started from `main.cpp`. |
| [`webservice_request_handling_flow.md`](dev_notes/webservice_request_handling_flow.md) | The practical path from a browser request through middleware, routing, and controllers. |
| [`websocket_cleanup_flow.md`](dev_notes/websocket_cleanup_flow.md) | WebSocket error handling, registry cleanup, and connection shutdown. |

## Application server

These guides explain how to configure, run, and extend the application server under [`src/app_server`](src/app_server/) .

| Document | What it covers |
| --- | --- |
| [`app_configuration.md`](src/app_server/doc/app_configuration.md) | Runtime and embedded configuration, fallback loading, variables, and the effective configuration object. |
| [`app_resource.md`](src/app_server/doc/app_resource.md) | Embedded and filesystem resources, resource contexts, build flags, and resource generation. |
| [`configuration_reference.md`](src/app_server/doc/configuration_reference.md) | The available `appsettings.json` objects and settings. |
| [`database_setup.md`](src/app_server/doc/database_setup.md) | SQLite setup, other database drivers, startup migrations, and troubleshooting. |
| [`endpoints.md`](src/app_server/doc/endpoints.md) | Registered pages, APIs, administration routes, optional module routes, and authentication defaults. |
| [`server_architecture.md`](src/app_server/doc/server_architecture.md) | Server startup, request flow, middleware, controllers, HTTPS, TLS, and optional modules. |
| [`server_mvc.md`](src/app_server/doc/server_mvc.md) | The MVC-style split between controllers, repositories, database services, views, and API results. |

## Core modules

These module-level guides describe the main reusable libraries in the project.

| Module | Document | What it covers |
| --- | --- | --- |
| Tobasa core | [`src/tobasa/README.md`](src/tobasa/README.md) | Shared utilities, config, logging, file helpers, and the common framework foundation. |
| Tobasa Web | [`src/tobasaweb/README.md`](src/tobasaweb/README.md) | Web framework concepts, routing, middleware, controller patterns, and app structure. |
| Tobasa LIS | [`src/tobasalis/README.md`](src/tobasalis/README.md) | LIS2-A2 and HL7 instrument communication, protocol handling, and device connectivity. |

## Tobasa HTTP

These documents cover the HTTP library, its server and parser, multipart
request handling, connection lifecycle, and WebSocket usage.

| Document | What it covers |
| --- | --- |
| [`README.md`](src/tobasahttp/README.md) | Library overview, protocols, features, dependencies, and architecture. |
| [`http_server.md`](src/tobasahttp/doc/http_server.md) | Server capabilities, configuration, and the boundary between the transport library and application code. |
| [`http_server_connection_handling.md`](src/tobasahttp/doc/http_server_connection_handling.md) | Accepting clients, HTTP/1 and HTTP/2 request processing, timeouts, WebSockets, SSE, and connection cleanup. |
| [`http_parser.md`](src/tobasahttp/doc/http_parser.md) | Incremental HTTP/1.x parsing, message framing, parser lifetime, and results. |
| [`parsing_multipart_internally.md`](src/tobasahttp/doc/parsing_multipart_internally.md) | How the HTTP server parses multipart form fields and uploaded files internally. |
| [`parsing_multipart_with_middleware.md`](src/tobasahttp/doc/parsing_multipart_with_middleware.md) | Multipart parsing through `MultipartMiddleware` and asynchronous request-body reading. |
| [`work_with_websocket.md`](src/tobasahttp/doc/work_with_websocket.md) | Managing WebSocket clients and connecting a `WebSocketContext` to request handling. |

## Tobasa SQL

These guides are for applications using the Tobasa SQL library directly.

| Document | What it covers |
| --- | --- |
| [`quick_start.md`](src/tobasasql/doc/quick_start.md) | Backend selection, direct connections, configured services, parameters, queries, results, and logging. |
| [`data_types.md`](src/tobasasql/doc/data_types.md) | Portable `tbs::sql::DataType` values and backend-specific type mappings. |
| [`sql_parameter.md`](src/tobasasql/doc/sql_parameter.md) | Parameter order, names, binding rules, and backend-specific behavior. |
| [`prepared_statement.md`](src/tobasasql/doc/prepared_statement.md) | The prepared-statement flow in `SqlQuery`, the driver `*Command` classes, and the one-shot execution pattern. |
| [`README.md`](src/tobasasql/README.md) | Module overview, features, supported databases, dependencies, and usage patterns. |

## Samples

These README files explain the sample applications under `src/samples`.

| Sample | What it covers |
| --- | --- |
| [`app_client/README.md`](src/samples/app_client/README.md) | A client for the app-server REST API, including HTTP/HTTPS requests, JSON data, authentication, resources, and WebSocket connections. |
| [`http_server/README.md`](src/samples/http_server/README.md) | HTTP and HTTPS listeners, request dispatch, `wwwroot` files, uploads, and WebSocket handling. |
| [`https_client/README.md`](src/samples/https_client/README.md) | HTTPS/TLS client connections, GET and POST requests, JSON payloads, certificate validation, and connection pooling. |
| [`https_server_minimal/README.md`](src/samples/https_server_minimal/README.md) | A minimal HTTPS server with request handling using a worker pool or the I/O thread. |
| [`tobasasql/README.md`](src/samples/tobasasql/README.md) | Direct SQL connections, runtime database configuration, services, transactions, connection pools, and MySQL checks. |

## Suggested starting points

- To run or configure the application server, start with
  [`server_architecture.md`](src/app_server/doc/server_architecture.md) and
  [`app_configuration.md`](src/app_server/doc/app_configuration.md).
- To add or understand an HTTP route, read
  [`endpoints.md`](src/app_server/doc/endpoints.md) and
  [`server_mvc.md`](src/app_server/doc/server_mvc.md).
- To trace a request through the implementation, use the notes in
  [`dev_notes`](dev_notes/).
- To use the SQL library in another application, start with
  [`quick_start.md`](src/tobasasql/doc/quick_start.md), then read
  [`data_types.md`](src/tobasasql/doc/data_types.md),
  [`sql_parameter.md`](src/tobasasql/doc/sql_parameter.md), and
  [`prepared_statement.md`](src/tobasasql/doc/prepared_statement.md).
- To see module-level background and usage notes, read the module README files
  for [`tobasa`](src/tobasa/README.md), [`tobasahttp`](src/tobasahttp/README.md),
  [`tobasaweb`](src/tobasaweb/README.md), [`tobasadicom`](src/tobasadicom/README.md),
  [`tobasalis`](src/tobasalis/README.md), and [`tobasasql`](src/tobasasql/README.md).
- To see working application examples, browse the
  [`src/samples`](src/samples/) README files listed above.
