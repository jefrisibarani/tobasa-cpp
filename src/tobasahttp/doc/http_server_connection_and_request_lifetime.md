# HTTP Server Connections and Request Lifetime

The listener accepts TCP sockets and gives each socket to a server connection. A connection reads requests, calls your handler, and writes responses. The connection manager keeps active connections alive and removes them after closure or failure.

Your handler receives one `HttpContext`. It reads the request, prepares the response, and tells the server whether processing is finished:

```cpp
http::RequestStatus handle(const http::HttpContext& context)
{
   context->response()->content("hello");
   context->response()->httpStatus(http::StatusCode::OK);
   return http::RequestStatus::handled;
}
```

`handled` means the response is ready to send. `notHandled` means the server should produce a not-found response. `async` means processing is still in progress; the handler or middleware must complete the context later.

## Connection and request lifetimes

The important difference is the scope of a connection:

- With HTTP/1, one TCP connection handles requests one at a time. Each request gets a context. After the response is sent, the server may reset the parser and use the connection for another request.
- With HTTP/2, one TCP connection owns an HTTP/2 session. Each request has its own stream and context, so several streams can remain active on the same connection.

The connection manager retains each active connection. It also handles shutdown, socket errors, and timers. HTTP/1 defaults to keep-alive for HTTP/1.1 unless the request asks to close. The server closes a connection after 100 requests by default; set `maxRequestsPerConnection(0)` to remove this limit.

Plain TCP connections use HTTP/1. HTTP/2 is available only when built and enabled, and is selected after a TLS handshake through ALPN. Otherwise a TLS connection uses HTTP/1.

## Process an HTTP/1 request

`ServerConnection` reads bytes and feeds them to the parser. The parser collects the request line and headers, then consumes the body using `Content-Length` or chunked framing. With internal multipart parsing enabled, multipart parts are also parsed before the handler runs.

The server calls the handler when the request is ready. The handler must prepare the response before returning `handled`. If no handler accepts the request, `notHandled` becomes `404 Not Found`.

For asynchronous work, copy the shared `HttpContext` into the operation and complete it when the response is ready:

```cpp
auto savedContext = context;
startAsyncWork([savedContext]
{
   savedContext->response()->content("finished");
   savedContext->response()->httpStatus(http::StatusCode::OK);
   savedContext->complete(http::RequestStatus::handled);
});

return http::RequestStatus::async;
```

`async` means the server will not write a response yet. Complete the context once; `complete()` invokes the completion callback installed by the server. Do not use references to stack data in deferred work. While the request is pending, the HTTP/1 connection cannot move on to its next request.

After the response is sent, the server removes request-owned multipart files. It reuses the connection only if keep-alive is enabled and the configured request limit has not been reached. The default limit is 100 requests; zero disables the limit. HTTP/1 parser errors produce an error response and close the connection.

## Process an HTTP/2 request

The HTTP/2 connection owns one `Http2Session`. HEADERS and DATA frames create and fill stream state. When a request is ready, the server creates a separate `HttpContext` for that stream and calls the application handler. The response is sent as frames on that same stream.

Each stream completes independently. Returning `async` leaves that stream's response pending; completing its context resumes the response path for that stream. Ending a stream does not close the TCP connection, and other streams can continue on it. HTTP/2 is selected only when it is built, enabled, and negotiated through TLS ALPN; plain TCP uses HTTP/1.

## Keep WebSocket and SSE lifetimes separate

A WebSocket or SSE response outlives a normal request/response exchange. Its transport scope depends on the HTTP version:

- With HTTP/1, a WebSocket upgrade or open SSE response holds the TCP connection. That connection cannot process another normal HTTP/1 request until the long-lived response closes.
- With HTTP/2, each WebSocket or SSE response uses a stream. Other streams can continue on the same TCP connection.

For HTTP/1 WebSocket, the request handler must accept the upgrade and attach a `WebSocketContext`; the server then sends `101 Switching Protocols`. HTTP/2 uses extended `CONNECT` for WebSocket and does not upgrade the whole connection. See [work_with_websocket.md](work_with_websocket.md) for the context and client APIs.

SSE is a response that stays open while events are sent. The server uses chunked transfer on HTTP/1 and one stream on HTTP/2. Closing the SSE connection ends that response or stream. The connection and event APIs are declared in [sse.h](../include/tobasahttp/sse.h). The application must close the WebSocket or SSE handle when the feature is finished; the server only closes it on transport shutdown or failure.

To tune read, write, and processing limits, see [server/settings.h](../include/tobasahttp/server/settings.h).

