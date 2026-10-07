# HTTP Server Settings

## How Settings Work

`http::Settings` and `http::SettingsTls` are configuration values. `Settings`
configures a plain HTTP server. `SettingsTls` adds TLS options and has the same
common HTTP settings. The server takes its settings by value when it is created,
then uses that owned copy for its listener and connections. Configure the value
before creating the server; changing your original value later does not change
the running server.

The setters form a fluent API. For example:

```cpp
http::Settings settings("127.0.0.1", 8084);
settings
   .timeoutRead(30)
   .timeoutProcessing(90)
   .maxHeaderSize(128 * 1024)
   .enableMultipartParsing(true);

http::PlainServerDefault server(io, settings, logger);
```

The important distinction is between settings that limit work and settings
that size buffers. A larger read buffer does not set a maximum request-body
size. Likewise, `clientBodyMaxSize()` is present in the API, but is not
currently passed to either HTTP parser, so it does not enforce a body limit.

## Address and port

The plain settings constructor defaults to `127.0.0.1:8084` with IPv4. The TLS
settings constructor defaults to `127.0.0.1:8085`, also with IPv4. The address
must be a numeric IP address: the listener calls `asio::ip::make_address()` and
does not resolve host names. Use an address such as `127.0.0.1`, `::1`, or
`0.0.0.0`.

The listener binds and starts accepting connections in `server.start()`. An
invalid address or a bind/listen failure throws `std::runtime_error` from
`start()`.

Both the constructor and `port()` setter map `0` to port `8084`. These settings
do not provide a way to request an operating-system-assigned port.

## Timeouts

Timeouts are in seconds. The setter accepts values from 10 seconds through one
hour. A value of `0` selects the implementation's 24-hour timeout; it does
not disable the timeout. Any other value outside the accepted range resets to
that timeout's default.

| Setting | Default | Accepted values | What it covers |
| --- | ---: | ---: | --- |
| `timeoutRead()` | 60 s | 10 s to 1 h, or `0` for 24 h | Waiting for request data from the socket. |
| `timeoutWrite()` | 60 s | 10 s to 1 h, or `0` for 24 h | Writing a response to the socket. |
| `timeoutProcessing()` | 120 s | 10 s to 1 h, or `0` for 24 h | Time spent waiting for the request handler to finish. |

For example, `timeoutRead(5)` is not a five-second timeout. It is invalid and
resets to 60 seconds. The processing timer applies while the handler is
working; asynchronous handlers must complete the request before it expires.
See [connection and request lifetime](http_server_connection_and_request_lifetime.md)
for how asynchronous completion works.

## Buffers and header limit

All sizes here are bytes. Read and send buffer setters accept 512 bytes through
8 MiB. The header-size setter accepts 512 bytes through 1 MiB. Values outside
those ranges reset to 64 KiB; they are not clamped to the nearest limit.

| Setting | Default | Effect |
| --- | ---: | --- |
| `readBufferSize()` | 64 KiB | Sizes the per-connection read buffer and sets the accepted socket's receive-buffer option. |
| `sendBufferSize()` | 64 KiB | Sets the accepted socket's send-buffer option and is also used when serializing HTTP/1 output. |
| `maxHeaderSize()` | 64 KiB | Limits the parsed request headers. For HTTP/2, it also limits the accumulated headers for a stream. |

These are not exact memory-use limits for the whole connection. For example,
the read buffer is one allocation per connection, while request bodies and
responses have their own storage and processing paths. The library does not
currently enforce `clientBodyMaxSize()` as a request-body limit.

## Connection and request limits

`maxRequestsPerConnection()` defaults to 100. It limits the number of requests
served on one HTTP/1 connection. Set it to `0` to remove that limit. It does
not limit the total number of connections or requests across the server.

The setter parameter is `uint16_t`. Although the source comment says the
maximum is 100,000, the implementation's effective accepted range is 0 through
34,464; an out-of-range value resets to 100. Keep values within that effective
range.

`useRateLimiter()` is off by default. When enabled, the listener counts newly
accepted TCP connections by remote IP address. It does not count HTTP requests
on a reused keep-alive connection. The defaults and accepted ranges are:

| Setting | Default | Accepted values |
| --- | ---: | ---: |
| `rateLimiterMaxRequests()` | 10 | 1 to 200 connections per time window |
| `rateLimiterWindowDuration()` | 1,000 ms | 1 ms to 1 hour |
| `rateLimiterBlockDuration()` | 30,000 ms | 1 ms to 24 hours |
| `rateLimiterMaxViolations()` | 3 | 1 to 1,000 violations |

Invalid values reset to the default. Once an IP exceeds the connection limit,
the listener closes its new socket without creating an HTTP connection, so the
client does not receive an HTTP error response. The first violations cause a
temporary block; reaching `rateLimiterMaxViolations()` permanently
blacklists that IP for the lifetime of the listener. This is an in-memory
connection limiter, not an application-level account or request quota.

## Request parsing and WebSocket messages

Multipart parsing is enabled by default. `enableMultipartParsing(false)` leaves
body parsing to application code or middleware; it does not install a
replacement body reader. `temporaryDir()` defaults to `./tmp` and selects the
directory used for multipart file parts. The server process must be able to
create and write there. Multipart files are temporary and are cleaned up with
the request. See [internal multipart parsing](parsing_multipart_internally.md)
and [multipart middleware parsing](parsing_multipart_with_middleware.md).

`wsMessageMaxSize()` defaults to 1 MiB. Valid values are 4 KiB through 10 MiB;
other values reset to 1 MiB. This limits received WebSocket messages, not HTTP
request bodies.

`clientBodyMaxSize()` defaults to `0` (documented in the header as unlimited).
The setter stores up to 50 MiB and maps `0` to that default, but the server
does not currently use the stored value when parsing HTTP/1 or HTTP/2 bodies.
Do not rely on this setting to protect an endpoint from large request bodies.
Enforce body limits in application code or middleware until the parser is wired
to this setting.

## Response compression

Compression is enabled by default. `compressionMinimalLength()` defaults to
1,024 bytes; values below 1,024 are reset to 1,024. `compressionEncoding()`
defaults to `"gzip"`. `compressionMimeTypes()` defaults to a list of selected
text and XML types and accepts a space-separated list that replaces that
default.

The setting only makes a response eligible for compression; the request and
response must also pass the server's checks. MIME matching and
`Accept-Encoding` handling have details that are easy to miss, so see
[HTTP response compression](http_server_compression.md) before relying on
compression for a particular response type.

## Logging and HTTP/2

`logVerbose()` defaults to `false` and enables additional HTTP connection
logging. When the library is built with `TOBASA_HTTP_USE_HTTP2`,
`http2Enabled()` defaults to `true` and `logVerboseHttp2()` defaults to
`false`. HTTP/2 is negotiated only for TLS connections that support it; plain
HTTP connections use HTTP/1. Disabling `http2Enabled()` makes TLS connections
use HTTP/1 instead. The option has no effect in builds without HTTP/2 support.

## TLS settings

`SettingsTls` inherits all settings above. Its TLS-specific settings are:

| Setting | Default | Purpose |
| --- | --- | --- |
| `certificateChainFile()` | Empty | Path to the certificate chain file. |
| `privateKeyFile()` | Empty | Path to the private key file. |
| `privateKeyPassword()` | Empty | Password for an encrypted private key. |
| `tmpDhFile()` | Empty | Path to a temporary Diffie-Hellman parameter file. |
| `hostCertificates()` | Empty list | Additional certificates selected by SNI hostname. Each `HostCertificate` has a `hostname`, `certificateChainFile`, `privateKeyFile`, and `password`. |
| `defaultTlsAssetCallback()` | Empty callback | Supplies certificate-chain, private-key, or DH bytes when the configured file path does not exist. |
| `serverMode()` | `true` | Currently stored but not read by the server's TLS setup; changing it does not change the server mode. |

For a standalone TLS server, provide usable certificate and private-key files,
or provide a callback that supplies those assets when the file paths do not
exist. The callback is also used for DH parameters when the DH file path does
not exist. With `hostCertificates()`, the server selects a matching entry by
the client's SNI hostname; missing per-host certificate or key files use the
default assets. Keep the bytes returned by the callback valid while TLS setup
loads them. Certificate provisioning is the application's responsibility. See
[`settings_tls.h`](../include/tobasahttp/server/settings_tls.h) and the
[HTTP server guide](http_server.md) for server construction details.

## Setter behavior and references

Most bounded numeric setters accept an in-range value and reset to their
documented default for an out-of-range value. They do not all share one generic
policy: for example, `clientBodyMaxSize()` caps values above 50 MiB, while
`compressionMinimalLength()` raises smaller values to 1,024 bytes. Check the
individual setting descriptions above when passing values from external
configuration.

The declarations are in
[`settings.h`](../include/tobasahttp/server/settings.h), with shared endpoint,
timeout, buffer, multipart, and HTTP/2 settings in
[`tobasahttp/settings.h`](../include/tobasahttp/settings.h). The behavior is
implemented by the server listener, connections, parsers, and response path;
the related guides linked above describe those paths in more detail.
