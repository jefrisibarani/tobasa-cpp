# Application Server: Architecture, Features, and TLS

The application server is the HTTP service built by `app_server`. It combines
the Tobasa HTTP server with the Tobasa web-service layer, controllers,
middleware, database services, authentication, static resources, and optional
modules such as LIS.

The server creates two listeners during startup:

- a plain HTTP listener on `webapp.httpServer.address` and `port`;
- a TLS listener on the same address and `portHttps`.

When `webapp.httpServer.runHttpsOnly` is `true`, only the TLS listener is
started. Otherwise both listeners are started. Both listeners use the same
web-service request pipeline.

## Request lifecycle

The high-level startup sequence is:

1. Load `appsettings.json`, using the embedded configuration as a fallback.
2. Deserialize the `webapp` and `logging` options.
3. Configure the database service and run registered database migrations.
4. Register middleware and controllers.
5. Initialize the router, including configured authentication rules.
6. Construct the HTTP and HTTPS servers.
7. Start the configured listeners and worker/I/O pools.

Requests are passed through the web-service middleware manager. The configured
middleware includes exception handling, database connectivity checking,
session handling, authentication/authorization, request identification, and
optional multipart parsing. The router then selects a controller handler by
HTTP method and path.

See [`endpoints.md`](endpoints.md) for the route inventory and
[`app_configuration.md`](app_configuration.md) for the configuration loading
model.

## Related configuration

The server settings are under `webapp.httpServer` in
`configuration/appsettings.json`:

| Setting group | Controls |
| --- | --- |
| `address`, `port`, `portHttps` | Listener bind address and ports. |
| `runHttpsOnly` | Whether the plain HTTP listener is started. |
| `http2Enabled` | HTTP/2 on the TLS listener when HTTP/2 support is compiled in. |
| `timeoutRead`, `timeoutWrite`, `timeoutProcessing` | Request timeout limits. |
| `readBufferSize`, `sendBufferSize`, `maxHeaderSize` | HTTP buffer and header limits. |
| `maxRequestsPerConnection` | Connection request limit. |
| `ioPoolSize`, `workerPoolSize` | I/O and request worker pool sizes. |
| `docRoot` | Filesystem document root used by the HTTP service. |
| `temporaryDir`, `enableMultipartParsing` | Multipart request processing. |
| `compression` | Response compression. |
| `useRateLimiter` and `rateLimiter*` | Optional request rate limiting. |
| `tls` | HTTPS certificate, key, DH, and SNI host configuration. |

The application resolves the temporary directory and TLS file paths relative
to the executable when the configured path is relative. An empty temporary
directory is replaced with the platform temporary directory, and the
application creates the resulting directory if it does not exist.

The server settings are deserialized into `tbs::http::conf::Server` and then
copied into `http::Settings` and `http::SettingsTls`. The detailed field
reference is in [`configuration_reference.md`](configuration_reference.md).

## Features

### HTTP and HTTPS

The server supports plain HTTP and TLS-enabled HTTPS. The TLS listener uses
the configured HTTPS port and can optionally enable HTTP/2 when the binary is
built with `TOBASA_HTTP_USE_HTTP2`.

#### Supported HTTP methods

At the HTTP connection layer, the server accepts these request methods:

| Method | Protocol status | Application use |
| --- | --- | --- |
| `GET` | Accepted | Used by page, API, administration, resource, LIS, and WebSocket route registrations. |
| `POST` | Accepted | Used by form, API, administration, LIS, and test route registrations. |
| `PUT` | Accepted | Used by the user profile update route. |
| `DELETE` | Accepted | Used by user and administration delete routes. |
| `HEAD` | Accepted by the HTTP parser | No application route is registered with `Router::httpHead`; support at the application route layer is not provided by `app_server`. |
| `OPTIONS` | Accepted by the HTTP parser | No application route is registered with `Router::httpOptions`; support at the application route layer is not provided by `app_server`. |

The server rejects `CONNECT`, `TRACE`, `PATCH`, and unknown methods during
request parsing with `405 Method Not Allowed`. For that response it sends an
`Allow` header containing the parser-level method list. Although the HTTP
method conversion utility defines enum values for `CONNECT`, `TRACE`, and
`PATCH`, those methods are intentionally not in the server's accepted-method
set.

The router's public registration API exposes only `httpGet`, `httpPost`,
`httpPut`, and `httpDelete`, which is why the application's route inventory
contains those four methods. A method being accepted by the parser does not
mean that every path has a handler for that method; consult
[`endpoints.md`](endpoints.md) for the registered method/path combinations.

### Routing and controllers

Routes are registered by controller factories during `WebService::setupHandlers`.
The router supports method-specific handlers and typed path parameters such as
`{user_id:int}`. A default handler serves unmatched routes through the core
controller.

### Middleware and authentication

Middleware is initialized before controllers and the router. Controller route
authentication declarations use `NONE`, `COOKIE`, `BEARER`, or `BASIC`; the
configured `routeAuthLists` can change the effective authentication decision
for matching paths. Cookie sessions and bearer-token authentication are both
used by the application.

### Static and embedded resources

The application can serve templates and static web-root resources from disk or
from resources compiled into the executable, depending on the build and
`webapp.webService.useInMemoryResources`. Configuration files and default TLS
assets have their own embedded resource group and are available independently
of the template/static-resource setting.

### Database-backed service

The web server is coupled to the configured application database. It runs the
database migration check before starting HTTP, and the database-check
middleware protects request processing when the database is unavailable. See
[`database_setup.md`](database_setup.md) for setup and migration details.

### WebSockets and optional modules

The HTTP routing layer can upgrade registered GET routes to WebSocket handling.
The test WebSocket controller and LIS controller are conditional build/module
features. They are not present in every executable.

### Operational controls

The server supports request timeouts, bounded connection request counts,
compression, multipart parsing, rate limiting, configurable thread pools,
custom status-page rendering, and graceful shutdown on `SIGINT`.

## TLS certificates

HTTPS uses one default TLS context and can create additional contexts for
configured SNI hostnames.

### Default certificate and key

The default settings come from `webapp.httpServer.tls`:

| Setting | Meaning |
| --- | --- |
| `certificateChainFile` | Default server certificate chain. |
| `privateKeyFile` | Default server private key. |
| `password` | Defined in the configuration model, but the current server setup does not pass it to `SettingsTls` as the private-key password. |
| `tmpDhFile` | Default temporary Diffie-Hellman parameter file. |

For each default asset, the HTTPS connection starter checks whether the
configured file exists. If it exists, that file is used. If it does not exist,
the application callback supplies an embedded asset:

| Asset | Embedded resource |
| --- | --- |
| Certificate chain | `tls_asset/127.0.0.1.crt` |
| Private key | `tls_asset/127.0.0.1.key` |
| DH parameters | `tls_asset/dh2048.pem` |

The embedded assets are compiled from the application `tls_asset` resources.
They provide a development fallback; they are not a replacement for a
deployment certificate issued for the server's real hostnames.

### Host-specific certificates and SNI

Add entries to `webapp.httpServer.tls.hostCertificates` to configure
hostname-specific certificates:

```json
"hostCertificates": [
	{
		"hostname": "example.test",
		"certificateChainFile": "./tls_asset/example.test.crt",
		"privateKeyFile": "./tls_asset/example.test.key",
		"password": ""
	}
]
```

At TLS setup time, the server creates an additional TLS context for each
entry and stores it by `hostname`. The TLS SNI callback selects that context
when the client supplies the matching hostname. If the client hostname has no
matching entry, TLS keeps using the default context.

For each host-specific entry independently:

- an existing `certificateChainFile` is loaded from disk;
- a missing certificate file falls back to the default embedded certificate;
- an existing `privateKeyFile` is loaded from disk;
- a missing key file falls back to the default embedded private key;
- the configured host `password` is used as the private-key password when it
	is non-empty;
- the configured/default DH file is used when it exists, otherwise embedded
	DH parameters are used.

The host-specific certificate paths are resolved relative to the executable
during application configuration loading. The default and host-specific
certificate entries are not automatically discovered from the filesystem;
they must be listed in configuration.

## What this server is not

The source defines this component as an application web service, so its scope
has important limits:

- It is not a general-purpose reverse proxy or load balancer. No upstream
	proxy routing is configured by the application server.
- It is not a database server. It connects to an existing server database or
	creates a local SQLite file, while the database engine performs the actual
	storage and query work.
- It is not a certificate authority. It loads certificate material and does
	not issue, renew, or obtain certificates.
- It is not a hot-reload configuration service. Configuration and TLS contexts
	are built during startup; editing configuration or certificate files does
	not reload them in a running process.
- It is not a static-file-only server. Static resources are one capability;
	requests also pass through middleware, authentication, routing, controllers,
	database services, and optional WebSocket/LIS handlers.
- It is not automatically HTTP/2 capable in every build. HTTP/2 settings are
	compiled conditionally and only affect the HTTPS server when that support is
	present.
- It is not a guarantee that the embedded TLS certificate matches a requested
	hostname. The embedded default assets are development fallbacks; use
	host-specific certificates for deployed names.

## Shutdown and failure behavior

The server listens for `SIGINT` and stops the secure listener alone in
HTTPS-only mode, or both listeners otherwise. It then shuts down the web
service and joins the I/O and worker threads.

Configuration, TLS setup, and server-start exceptions are logged and prevent a
normal running server. A database migration connection failure is logged by
the migration subsystem; the later database connectivity checks determine
whether startup can proceed successfully.
