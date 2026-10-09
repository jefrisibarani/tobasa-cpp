# Tobasa HTTP Client Internals

This note describes how the HTTP client is put together and how a request moves
through it. The implementation is HTTP/1 over TCP or TLS. HTTP/2 support in
Tobasa currently applies to the server, not this client.

## Main components

The public client types are aliases in
[`http_client.h`](../src/tobasahttp/include/tobasahttp/client/http_client.h).
They combine `Client` with traits that select the settings, logger, socket,
and connection starter.

- [`Client`](../src/tobasahttp/include/tobasahttp/client/client.h) is the
	coordinator. It owns an `io_context`, settings, a connector, a connection
	manager, an idle-connection pool, and a queue of waiting asynchronous
	requests.
- [`ClientRequest`](../src/tobasahttp/include/tobasahttp/client/request.h)
	holds the outbound method, complete target, headers, body, and HTTP version
	metadata. `Client::buildRequest()` adds `Host`, keep-alive, and default
	headers to the same `ClientRequest` before it reaches the connection.
- [`RequestSerializer`](../src/tobasahttp/include/tobasahttp/client/request_serializer.h)
	serializes a `ClientRequest` as an HTTP/1 request. The server-side `Request`
	model is not used by the client.
- [`Connector`](../src/tobasahttp/include/tobasahttp/client/connector.h)
	resolves the configured address and opens a TCP connection. The plain
	connection starter passes the socket through directly; the TLS starter wraps
	it in an SSL stream and performs the TLS handshake.
- [`ClientConnection`](../src/tobasahttp/include/tobasahttp/client/client_connection.h)
	owns one socket, serializes requests, parses responses, and calls the
	response handler for the request currently in flight on that connection.
- [`ConnectionMgr`](../src/tobasahttp/include/tobasahttp/connection_manager.h)
	assigns connection IDs, tracks live connections, and reports connection
	errors, timeouts, and closure.

One connection processes one request at a time. When an HTTP/1 response allows
keep-alive and the request limit has not been reached, the connection resets
its response parser and returns to the client pool. The next request gets its
own response handler before it is written.

## Synchronous requests

A synchronous call such as `get(path)` builds a `ClientRequest` and enters the
synchronous `execute()` overload. It returns a `ClientResult` with either the
response or the error that prevented a response. `responseReceived()` indicates
whether a response arrived; it does not indicate whether the HTTP status is
successful. The client selects synchronous mode and rejects attempts to mix it
with asynchronous mode.

The client posts the work to its executor, then runs its internal I/O context
on the calling thread. It takes an idle connection from the pool if one is
available. Otherwise it resolves and connects to the configured host, performs
the TLS handshake when needed, and starts the connection. When the response or
request error arrives, the result is stored and the internal I/O context stops.
The call then returns its `ClientResult`.

Repeated synchronous calls can reuse a keep-alive connection, but they still
run one at a time. Increasing the pool size does not make synchronous calls
concurrent.

## Asynchronous requests

An asynchronous call takes a `ClientResultHandler`. The client dispatches request
scheduling onto its client strand, where it chooses one of three paths:

1. Take an idle connection and send the request on it.
2. If the pool has room, start a new resolve/connect operation.
3. If all allowed connections are busy, append the request and its result handler to
	 the pending queue.

When a request completes or fails, its result handler receives that request's
`ClientResult`. The connection then posts itself back to the client strand.
The client either starts the oldest pending request on that connection or
marks the connection idle in the pool. A connection error or failed connect
frees a pool slot and allows queued work to proceed.

Call `shutdown()` after submitting work, then call `run()`. Shutdown rejects
later submissions and internally drains all accepted requests, including
queued requests and their result handlers, before closing connections and
releasing the work guard. `run()` then returns. With the default argument,
`run()` uses the calling thread. With a larger thread count, it starts that
many threads running the same I/O context and joins them. Use `stop()` for
immediate cancellation instead of draining.

## Strands and worker threads

The client has one strand for scheduling, pool contents, the pending queue,
and the active-connection count. Connection lifecycle notifications are posted
to this strand before those values are changed.

Each connection has its own strand for its socket operations and response
handling. This keeps operations on one connection in order while allowing
different connections to make progress on separate I/O workers. The worker
count is not a thread-per-connection setting: all workers share one
io_context`, and Asio schedules ready operations on them. Application result
handlers can run concurrently when responses arrive on different connections;
protect any application state they share.

`connectionPoolSize()` limits the number of active client connections. It is
not the worker count. A synchronous client reuses connections serially; an
asynchronous client can use multiple connections at once and queues additional
requests while those connections are busy.

## Errors and lifetime

The connection manager owns the live connection set. When a connection reports
completion, error, or timeout, the manager removes or closes it and notifies
the client. The client forwards its public error callbacks and updates the
pool on its strand. A connect failure is reported through `onConnectFailed()`;
connection errors, timeouts, and closure have their corresponding callbacks.

The client and logger must outlive operations that use them. In asynchronous
mode, keep result-handler captures alive until callbacks run. Avoid blocking
work in a result handler: it runs on that connection's strand and delays
further work on the same connection. Global connection callbacks are optional
and intended for app-wide notifications; request-specific outcomes arrive in
each `ClientResult`.

## Code map

- Public aliases and client traits:
	[`http_client.h`](../src/tobasahttp/include/tobasahttp/client/http_client.h)
	and [`client_traits.h`](../src/tobasahttp/include/tobasahttp/client/client_traits.h).
- Mode selection, request scheduling, pool, and shutdown:
	[`client.h`](../src/tobasahttp/include/tobasahttp/client/client.h).
- Resolve and TCP connect:
	[`connector.h`](../src/tobasahttp/include/tobasahttp/client/connector.h).
- Per-connection request/response work:
	[`client_connection.h`](../src/tobasahttp/include/tobasahttp/client/client_connection.h).
- Regression coverage for pooled handlers and repeated sync requests:
	[`client_pool_test.cpp`](../src/tobasahttp/tests/unit/client_pool_test.cpp).
