# Run an HTTP Server

`tobasahttp` accepts HTTP requests and passes them to an application handler.
The handler decides what a request means and prepares the response; the server
takes care of the connection and HTTP protocol.

## Choose HTTP or HTTPS

Use `PlainServerDefault` for HTTP over TCP and `SecureServerDefault` for HTTPS.
Both are aliases declared in
[`http_server.h`](../include/tobasahttp/server/http_server.h).

The server uses HTTP/1 for plain connections. When built with
`TOBASA_HTTP_USE_HTTP2` and nghttp2, an HTTPS connection can negotiate HTTP/2
with TLS ALPN. The CMake option is enabled by default. HTTP/1 handles requests
in order on each connection; HTTP/2 can handle several independent streams on
one connection. HTTP/2 is not available over cleartext TCP.

The server accepts `GET`, `POST`, `PUT`, `DELETE`, `HEAD`, `OPTIONS`, and
`CONNECT`. This does not make it a general-purpose CONNECT tunnel. `TRACE` and
`PATCH` are not enabled.

## Start the server

The application owns the Asio `io_context`. `start()` begins listening and
returns; call `io.run()` to process network activity. The server does not
create worker threads, so an application can choose to run the context on one
or more threads.

```cpp
asio::io_context io;
http::Settings settings("127.0.0.1", 8084);
http::PlainServerDefault server(io, std::move(settings), logger);

server.requestHandler([](const http::HttpContext& context) {
   context->response()->content("Hello");
   context->response()->httpStatus(http::StatusCode::OK);
   return http::RequestStatus::handled;
});

server.start();
io.run();
```

The listener expects a numeric IP address such as `127.0.0.1`, `::1`, or
`0.0.0.0`; it does not resolve host names. Configure settings before creating
the server because the server keeps its own copy. For HTTPS, use
`SettingsTls` and provide a certificate chain and private key. TLS settings
also support SNI certificates and loading TLS assets from memory. See
[`settings.md`](settings.md) for settings and limits.

## Handle requests

The handler receives an `HttpContext` with the request, response, connection
information, and application data. Return `RequestStatus::handled` after
preparing a response. `notHandled` sends a not-found response. Return `async`
when work will finish later, then complete the context when the response is
ready. See [connection and request lifetime](http_server_connection_and_request_lifetime.md)
for the asynchronous flow.

HTTP/1 requests are parsed incrementally. A request body can use
`Content-Length` or chunked transfer encoding. Multipart form data is parsed
internally by default, including uploaded files; applications can disable that
path and let middleware read the body instead. The parser also has a maximum
header size, but `clientBodyMaxSize()` is not currently enforced. Apply a body
limit in the handler or middleware if the application needs one.

Responses can contain strings, files, or raw bytes. Compression is enabled by
default for eligible responses and uses gzip. File responses can opt in to
single byte-range requests, which allow clients to resume downloads. For the
details, see [compression](http_server_compression.md) and
[resumable downloads](http_server_resumable_download.md).

The request context can also start an SSE or WebSocket connection when the
application supplies its context and callbacks. HTTP/1 WebSockets use the
Upgrade handshake; HTTP/2 WebSockets use extended CONNECT. See
[WebSocket connections](work_with_websocket.md) for the handshake and callback
lifecycle.

## Stop the server

Call `server.stop()` while the I/O context is running to close the listener
and active connections. If the application uses multiple threads, post the
stop call to `server.executor()`. The server can also report active connection
counts and connection information through `totalConnections()`,
`lastConnectionId()`, and `currentConnectionsInfo()`.

The optional rate limiter checks new TCP connections by remote IP address; it
does not limit requests sent over an existing keep-alive connection. The
server does not provide routing, automatic static-file serving, or
application-level authentication. Validate request paths before using them to
open files, and enforce access rules in application code.

For complete examples, see the [HTTP server sample](../../samples/http_server/src/main.cpp)
and [minimal HTTPS server sample](../../samples/https_server_minimal/src/without_worker_threads.cpp).
