# HTTP Request Handling Flow

This describes the HTTP/1.x path implemented by `ServerConnection`.

## Browser Sends a Request

1. The browser opens a TCP connection and sends an HTTP request.

2. The listener accepts the socket and creates a `ServerConnection`.

3. The listener's connection-created callback gives the connection to
   `ConnectionMgr`.

4. `ConnectionMgr::addConnection()`:

   - installs timeout, error, completion, and closed handlers;
   - allocates and assigns a `ConnectionId`;
   - inserts the connection into its `_connections` registry;
   - calls `conn->start()`.

   Registering before `start()` ensures an immediate startup error can still
   find the connection in the manager registry.

## Start Reading

5. `ServerConnection::start()` initializes the parser request ID and starts
   the processing stopwatch.

6. The connection's `onStart` callback selects the protocol:

   - HTTP/1.x calls `startHttp1()`;
   - HTTP/2 calls `startHttp2()` when HTTP/2 support is enabled.

7. `startHttp1()` calls `read()`.

8. `read()` starts the read timeout and calls
   `socket.async_read_some()` with the connection read buffer.

## Parse Incoming Bytes

9. When bytes arrive, the read callback:

   - clears the parser reading flag;
   - checks the socket error and closed state;
   - updates `_totalBytesTransferred`;
   - passes the bytes to the HTTP parser.

10. If parsing fails, `handleRequestError()` creates an error context,
   prepares an error response, sets `Connection: close`, and writes the error
   response.

11. If more bytes are required, `read()` is called again.

12. If the request uses `Expect: 100-continue`, the server sends the interim
   response and resumes reading.

13. When headers or content become available, `retrieveRequest()` creates a
   new `Context` for a new request. It initializes the request metadata,
   moves parsed headers into the request, and determines keep-alive behavior.

14. For multipart requests, the code may create a `MultipartBodyReader` and
   transfer body reading to it. The request handler is then invoked while the
   reader supplies the remaining body data.

## Request Complete

15. When the parser reports `contentDone()`, the parsed body is moved into the
   request.

16. If the request has an `Upgrade` header, processing moves to
   `handleUpgradeRequest()` for WebSocket or another supported upgrade.

17. Otherwise, `handleRequest()` is called.

18. `handleRequest()`:

   - logs the request;
   - installs the context completion handler;
   - installs the SSE initialization handler;
   - invokes the configured application request handler.

19. The application handler returns a `RequestStatus`:

   - `notHandled`: the server builds a `404 Not Found` response and writes it;
   - `handled`: the server starts the SSE response if `_sseState` exists,
    otherwise it writes the normal HTTP response;
   - `async`: the server waits for middleware or application code to continue
    the request through the context completion handler.

## Write the HTTP Response

20. `write()` serializes the response into `_sendBuffer`.

21. Compression is prepared when needed, based on the request's
   `Accept-Encoding` header.

22. The server starts the write timeout and calls `asio::async_write()`.

23. The write callback:

   - updates the response transfer count;
   - consumes written bytes from `_sendBuffer`;
   - continues writing when chunked data, compression, or a response data
    source has remaining bytes;
   - handles write errors through `processError()`;
   - otherwise calls `handleKeepAliveOrClose()`.

## Keep-Alive or Finish

24. `handleKeepAliveOrClose()` checks the response keep-alive setting and the
   maximum request limit.

25. If the connection remains persistent, the parser is prepared for the next
   message, the request ID is advanced, and `read()` starts the next request.

26. Otherwise, `processCompleted()` invokes the completion handler.
   `ConnectionMgr` removes the connection from its registry and closes the
   underlying socket.

```text
Browser sends HTTP request
        |
        v
Listener accepts socket
        |
        v
ConnectionMgr assigns ID, registers connection, calls start()
        |
        v
ServerConnection::start()
        |
        v
startHttp1() -> read() -> async_read_some()
        |
        v
HTTP parser consumes bytes
        |
        +--> parse error -> error response -> close
        +--> more bytes needed -> read again
        +--> 100-continue -> send interim response -> read again
        +--> multipart -> MultipartBodyReader -> handleRequest()
        +--> Upgrade -> WebSocket/SSE upgrade path
        |
        v
Request body complete
        |
        v
handleRequest() -> application request handler
        |
        v
RequestStatus
        |
        +--> notHandled -> 404 response
        +--> handled -> write response
        +--> async -> wait for continuation
        |
        v
async_write response
        |
        +--> remaining output -> write again
        +--> keep-alive -> prepare parser -> read next request
        +--> close -> processCompleted()
                       |
                       +--> ConnectionMgr removes connection
                       +--> socket closes
```
