# Tobasa HTTP Server

## What it is

Tobasa HTTP Server is a C++ library for accepting HTTP connections and passing
parsed requests to application code. It provides the network and HTTP
processing engine. Your application decides what each URL means and builds the
response.

The public server types are in
[`http_server.h`](../include/tobasahttp/server/http_server.h). Use
`PlainServerDefault` for HTTP or `SecureServerDefault` for HTTPS.

## Engine

The server uses standalone Asio for asynchronous TCP networking. It does not
create its own worker threads. The application owns an `asio::io_context`,
starts the server, and runs that context to process network work. One or more
threads can call `io_context.run()`; the sample server uses a thread pool.

At startup, `Server::start()` asks a listener to bind and listen. The listener
accepts clients asynchronously and creates a connection object for each one.
The connection manager tracks active connections. Each connection reads and
parses requests, calls the application handler, and sends the response.

HTTP/1 requests use Tobasa's HTTP parser. The parser supports request bodies
with a content length or chunked transfer encoding. When HTTP/2 is enabled at
build time, the HTTPS connection can negotiate HTTP/2 with TLS ALPN and uses
nghttp2 to process its streams. Plain TCP connections use HTTP/1; this code
does not negotiate HTTP/2 over cleartext TCP.

## What it supports

- HTTP/1 over TCP and HTTPS over TLS.
- HTTP/2 over HTTPS when built with `TOBASA_HTTP_USE_HTTP2` and enabled in
	settings. The CMake option is on by default, but the build also needs the
	nghttp2 dependency.
- The methods `GET`, `POST`, `PUT`, `DELETE`, `HEAD`, `OPTIONS`, and `CONNECT`.
	`TRACE` and `PATCH` are not enabled in the server's method list.
- Keep-alive connections. By default, a connection is closed after 100 HTTP/1
	requests; set `maxRequestsPerConnection(0)` to remove that limit.
- Multipart form parsing, including file parts. Temporary files use the
	configured temporary directory.
- Response compression, enabled by default for selected MIME types and
	responses of at least 1 KB.
- WebSocket and Server-Sent Events (SSE) handling through the request context.
	The application must provide the WebSocket or SSE context and callbacks.
- Read, write, and request-processing timeouts, plus configurable socket
	buffers and maximum header size.
- An optional per-IP connection rate limiter. It is off by default.

For HTTP/1, the server processes requests on a connection in order. With
HTTP/2, different request streams can be active on one connection.

## What it is not

- It is not a complete web framework. There is no built-in URL route table;
	register a request handler and dispatch paths in your application.
- It does not automatically serve a directory of static files. The sample
	implements file serving in its own handler.
- It does not supply application authentication, authorization, account
	management, or database rules. Add those in application code or middleware.
- It is not an HTTP/3 server. The implemented network paths are HTTP/1 and
	optional HTTP/2 over TLS.
- It does not resolve host names when binding. Although the settings
	constructors use `localhost` as their address text by default, the listener
	calls `asio::ip::make_address()`, which expects a numeric IP address. Set an
	address such as `127.0.0.1`, `::1`, or `0.0.0.0` explicitly.

## Configure it

`Settings` is for plain HTTP. `SettingsTls` adds TLS certificate and key
settings. Both use a fluent API. Common settings include:

| Setting | Default | Purpose |
| --- | --- | --- |
| Address and port | `localhost:8084` for `Settings`; `localhost:8085` for `SettingsTls` | Local endpoint. Use a numeric address as described above. |
| Read and write timeout | 60 seconds each | Maximum time for socket reads and writes. A value of `0` becomes a 24-hour timeout in the current implementation. |
| Processing timeout | 120 seconds | Maximum time allowed for request processing. A value of `0` becomes a 24-hour timeout in the current implementation. |
| Read and send buffer | 64 KB each | Socket buffer sizes. |
| Maximum header size | 64 KB | Header size limit. |
| Maximum requests per connection | 100 | HTTP/1 keep-alive request limit; `0` disables the limit. |
| Multipart parsing | Enabled | Parse multipart request bodies and file parts. |
| Temporary directory | `./tmp` | Location used for multipart temporary files. |
| Compression | Enabled | Compress eligible responses when the client accepts the configured encoding. |

The settings implementation clamps invalid timeout and buffer values to its
documented defaults or limits. Check
[`settings.h`](../include/tobasahttp/settings.h) and
[`server/settings.h`](../include/tobasahttp/server/settings.h) for the exact
limits and additional options.

For HTTPS, provide a certificate chain and private key file. The TLS settings
also support a private-key password, a temporary Diffie-Hellman file, and a
callback that supplies TLS assets from memory. The sample uses files beside
the executable; production certificate provisioning is the application's
responsibility.

Example settings:

```cpp
asio::io_context io;
http::Settings settings("127.0.0.1", 8084);
settings.maxRequestsPerConnection(100)
				.timeoutRead(60)
				.enableMultipartParsing(true);

http::PlainServerDefault server(io, std::move(settings), logger);
```

For HTTPS, use `http::SettingsTls`, set a numeric address, call
`certificateChainFile(...)` and `privateKeyFile(...)`, then create a
`http::SecureServerDefault`.

## Start and stop

Register one request handler. It receives an `HttpContext`, reads the request,
sets the response, and returns a `RequestStatus`:

```cpp
server.requestHandler([](const http::HttpContext& context) {
	 context->response()->content("Hello");
	 context->response()->httpStatus(http::StatusCode::OK);
	 return http::RequestStatus::handled;
});
```

Then start the server and run the I/O context:

```cpp
server.start();
io.run();
```

`start()` opens the listening socket and schedules asynchronous accepts. It
does not block and does not run the I/O context. To stop cleanly, call
`server.stop()` while the context is running; this closes the listener and
stops active connections. Applications commonly arrange a signal handler to
post `stop()` onto `server.executor()`. See the HTTPS example in
[`without_worker_threads.cpp`](../../samples/https_server_minimal/src/without_worker_threads.cpp)
and the combined HTTP/HTTPS example in
[`main.cpp`](../../samples/http_server/src/main.cpp).

## A few useful details

- `RequestStatus::handled` sends the response prepared by the handler.
	`notHandled` produces a not-found response. `async` lets the application
	finish request processing later by completing the context.
- In the optional rate limiter, the check runs as each TCP connection is
	accepted. Despite the setting names, it limits new connections per IP and
	time window; it does not count individual HTTP requests on a reused
	keep-alive connection.
- The library can track active connections and report their IDs and connection
	information through `totalConnections()`, `lastConnectionId()`, and
	`currentConnectionsInfo()`.
- The server validates supported methods, but request routing and important
	application checks remain yours. In particular, validate paths before
	opening files, and enforce access rules in the handler.
