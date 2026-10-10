# Send HTTP Requests with Tobasa

`http::Client` sends HTTP requests and returns the server's response. It
supports HTTP/1 over TCP and TLS. The client does not currently support
HTTP/2; optional HTTP/2 support applies to the TLS server.

## Make a request

Use `PlainClientDefault` for HTTP or `SecureClientDefault` for HTTPS. Configure
the client with `SettingsClient` before constructing it:

```cpp
#include <tobasahttp/client/http_client.h>

log::StdoutLogger logger;
http::SettingsClient settings("127.0.0.1", 8085);
settings.tlsMode(true);

http::SecureClientDefault client(std::move(settings), logger);
client.addHeader("Accept", "application/json");

auto result = client.get("/api/version");
if (result.responseReceived())
   std::cout << result.response->statusCode() << ": " << result.response->content() << '\n';
else
   logger.error("Request failed: {}", result.error.message);
```

The client also provides `post()` and `put()` calls. They use
`application/json` as the default content type; pass a content type when
sending another kind of body. Use `addHeader()` for headers that should be
sent with each request. Convenience methods build an outbound `ClientRequest`;
its target is sent in full, including any query string. The response contains
the status, headers, and body.

## Choose how requests run

Synchronous calls such as `get()` wait for completion and return a
`ClientResult`, which contains either the response or the request error. Use
`responseReceived()` to check whether a response arrived; HTTP error status
codes are still responses and must be checked separately.

For asynchronous calls, pass a `ClientResultHandler` and run the client:

```cpp
client.get("/api/version", [&](const http::ClientResult& result)
{
   if (result.responseReceived())
      std::cout << result.response->content() << '\n';
   else
      logger.error("Request failed: {}", result.error.message);
});

client.shutdown(); // stop submissions and drain all accepted requests
client.run();
```

`run()` can use more than one worker thread. The asynchronous client keeps
running until `shutdown()` is called. Call it after submitting all requests;
the client rejects later submissions, completes the accepted requests and
their result handlers, closes its connections, then lets `run()` return.
`stop()` remains available for immediate cancellation. Register
`onConnectionError()`, `onConnectFailed()`, and `onConnectionTimeOut()` when
the application needs global network error notifications. Do not mix
synchronous and asynchronous requests on the same client instance.

### Per-request and global errors

The `ClientResult` returned by a synchronous helper or delivered to an
asynchronous helper's `ClientResultHandler` contains that request's response
or error. Handle request-specific failures there. The global callbacks are
optional and are useful for centralized logging, metrics, or notifications
across requests; they do not replace the per-request result. They are not
needed to coordinate shutdown: call `shutdown()` after submitting work and
the client drains the accepted requests internally.

## Handle a request result

Use `execute()` for custom methods. It shares the same `ClientResult`
shape as the synchronous and asynchronous convenience methods:

```cpp
auto result = client.execute(http::ClientRequest("PATCH", "/api/item"));
if (result.responseReceived())
   std::cout << result.response->content() << '\n';
else
   logger.error("Request failed: {}", result.error.message);
```

`result.error` contains the transport or client error; non-success HTTP status
codes still arrive as responses. Client-level error callbacks remain
available for application-wide notifications.

The client reuses HTTP/1 keep-alive connections. Set `connectionPoolSize()` to
change the maximum number of connections; asynchronous requests wait when all
connections are busy. See [settings](settings.md) for the shared timeout and
connection settings.

## Stream large request and response bodies

Streaming is opt-in. A request without a body source or response-body handler
continues to use the existing buffered behavior.

Use a file or owned byte vector as an outbound body source:

```cpp
http::ClientRequest upload("PUT", "/archive");
upload.bodySource(http::RequestDataSource::fromFile("archive.tar"));
auto result = client.execute(std::move(upload));
```

For bytes, pass an owned `std::vector<uint8_t>` to
`RequestDataSource::fromBytes()`. The source reports its size and the client
sends it with `Content-Length`, reading and writing bounded blocks. It owns the
byte vector for the full transfer. Unknown-size producers and chunked request
uploads are not supported by this API. A caller-supplied `Transfer-Encoding`
header conflicts with this framing and is rejected.

To consume a response without keeping the entire body in memory, set a
synchronous response-body handler on the request:

```cpp
http::ClientRequest download("GET", "/large-file");
download.responseBodyHandler([&output](const uint8_t *data, size_t size)
{
   output.write(reinterpret_cast<const char *>(data),
      static_cast<std::streamsize>(size));
});
auto result = client.execute(std::move(download));
```

The handler receives decoded entity bytes for both `Content-Length` and
chunked responses; chunk framing and trailers are not delivered. Calls happen
synchronously on the connection executor, so the handler must not retain the
pointer and should not perform long blocking work. Copy or consume the bytes
before returning. The final `ClientResult` still arrives after the full
response; status and headers are available, while `response->content()` is
empty for a streamed response. Bytes already delivered remain consumed if a
later transport or framing error occurs, and that request completes with an
error rather than a response. A connection with a streamed body is not reused
until that response has been fully parsed.

For a determinate progress indicator, inspect response headers before body
delivery:

```cpp
std::size_t expectedBytes = 0;
std::size_t receivedBytes = 0;

http::ClientRequest download("GET", "/large-file");
download.responseHeadersHandler([&](const http::ClientResponsePtr& response)
{
   const auto length = response->headers().value("Content-Length");
   if (!length.empty())
      expectedBytes = static_cast<std::size_t>(std::stoull(length));
});
download.responseBodyHandler([&](const uint8_t *, std::size_t size)
{
   receivedBytes += size;
   // Publish receivedBytes / expectedBytes when expectedBytes is known.
});
```

The headers handler runs once after the response headers and framing have been
validated, before any body chunks, and receives the same response object later
delivered with the completed result. Chunked responses generally have no
`Content-Length`, so their total size may be unknown. Both handlers run
synchronously on the connection executor; marshal UI updates to the UI thread
and keep the callbacks brief.

## Use TLS

`SecureClientDefault` creates a TLS connection. Peer verification is off by
default. To verify the server certificate using OpenSSL's default CA paths,
enable verification:

```cpp
settings
   .tlsMode(true)
   .verifyPeer(true);
```

Set `caVerificationFile()` only when you need to trust an additional CA or
use a custom CA bundle.

For mutual TLS, also set `certificateFile()` and `privateKeyFile()` to the
client certificate and key. The HTTPS client sample in
[`main.cpp`](../../samples/https_client/src/main.cpp) shows a complete setup.

## Related guides

See the [HTTP server guide](http_server.md) for handling requests, the
[HTTP/1 parser guide](http_parser.md) for message parsing, and the
[build guide](../../../BUILD.md) for building the repository.