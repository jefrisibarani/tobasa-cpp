# Tobasa HTTP

`tobasahttp` is Tobasa's HTTP transport library. It handles network
connections, HTTP messages, and TLS so applications can focus on what each
request should do. It is used by higher-level components such as `tobasaweb`,
which provides routing and middleware.

## HTTP server

The server uses Asio for network communication. The application runs the
server's event loop. It handles HTTP/1 over plain HTTP or HTTPS, and can use
HTTP/2 over HTTPS when built with nghttp2. HTTP/2 over plain
HTTP and HTTP/3 are not supported.

- The server accepts `GET`, `POST`, `PUT`, `DELETE`, `HEAD`, `OPTIONS`, and
  `CONNECT`. `TRACE` and `PATCH` are not enabled.
- The HTTP/1 parser handles messages incrementally, with bodies framed by
  content length or chunked transfer encoding.
- Multipart form data, including file parts, can be parsed internally or left
  for application middleware.
- Responses can contain text, files, or raw bytes. Eligible responses can use
  gzip compression, and file downloads can opt in to single byte ranges.
- WebSockets and Server-Sent Events are supported. WebSockets use the HTTP/1
  Upgrade handshake or the HTTP/2 extended CONNECT path.
- TLS uses OpenSSL and supports certificates, private keys, SNI certificates,
  and in-memory TLS assets.
- Server settings include timeouts, header and socket-buffer sizes, multipart
  temporary files, and an optional per-IP connection limiter.

The module also provides helpers for headers, cookies, query strings, and
content types.

## HTTP client

The client sends HTTP/1 requests over plain HTTP or HTTPS and can reuse pooled
connections. It supports blocking and asynchronous requests, but does not
currently support HTTP/2.


## Building

Build `tobasahttp` as part of the repository's CMake project. See the
[build guide](../../BUILD.md) for dependency setup. The module uses Tobasa,
standalone Asio, OpenSSL, and zlib. nghttp2 is needed only when HTTP/2 support
is enabled; the `TOBASA_HTTP_USE_HTTP2` CMake option is enabled by default.

## Documentation and samples

The [HTTP server guide](doc/http_server.md) covers server setup and request
handling. The [HTTP client guide](doc/http_client.md) describes making
blocking and asynchronous requests. See the [HTTP/1 parser guide](doc/http_parser.md)
for message parsing, and the [settings guide](doc/settings.md) for defaults
and limits. Separate guides cover [compression](doc/http_server_compression.md),
[resumable downloads](doc/http_server_resumable_download.md), and
[WebSocket connections](doc/work_with_websocket.md).

Working examples are available in the [HTTP server sample](../samples/http_server/),
[minimal HTTPS server sample](../samples/https_server_minimal/), and
[HTTPS client sample](../samples/https_client/).

## License

GNU Lesser General Public License. See the repository's `LICENSE` file.

