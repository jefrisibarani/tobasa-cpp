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