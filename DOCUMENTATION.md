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

These guides explain how to configure, run, and extend the application server.

| Document | What it covers |
| --- | --- |
| [`app_configuration.md`](src/app_server/doc/app_configuration.md) | Runtime and embedded configuration, fallback loading, variables, and the effective configuration object. |
| [`app_resource.md`](src/app_server/doc/app_resource.md) | Embedded and filesystem resources, resource contexts, build flags, and resource generation. |
| [`configuration_reference.md`](src/app_server/doc/configuration_reference.md) | The available `appsettings.json` objects and settings. |
| [`database_setup.md`](src/app_server/doc/database_setup.md) | SQLite setup, other database drivers, startup migrations, and troubleshooting. |
| [`endpoints.md`](src/app_server/doc/endpoints.md) | Registered pages, APIs, administration routes, optional module routes, and authentication defaults. |
| [`server_architecture.md`](src/app_server/doc/server_architecture.md) | Server startup, request flow, middleware, controllers, HTTPS, TLS, and optional modules. |
| [`server_mvc.md`](src/app_server/doc/server_mvc.md) | The MVC-style split between controllers, repositories, database services, views, and API results. |

## TobasaSQL

These guides are for applications using the TobasaSQL library directly.

| Document | What it covers |
| --- | --- |
| [`quick_start.md`](src/tobasasql/doc/quick_start.md) | Backend selection, direct connections, configured services, parameters, queries, results, and logging. |
| [`data_types.md`](src/tobasasql/doc/data_types.md) | Portable `tbs::sql::DataType` values and backend-specific type mappings. |

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
  [`data_types.md`](src/tobasasql/doc/data_types.md).
- To see working application examples, browse the
  [`src/samples`](src/samples/) README files listed above.
