
# WebSocket Cleanup Flow

1. The browser closes the WebSocket connection.

2. The server's pending WebSocket read completes with `EOF`, `connection_reset`, or another socket error.

3. The read callback calls `WebSocketState::onError()`.

4. `WebSocketState::onError()`:
   - invokes the application error handler;
   - removes the `WebSocketPtr` from `WebSocketContext::_connections`;
   - resets `wsPtr`;
   - releases the WebSocket context reference.

5. The read callback calls `processError()`.

6. `ConnectionMgr::handleOnComplete()` handles `EOF` and `connection_reset`, then calls:

   ```cpp
   ConnectionMgr::stop(connectionId, reason);
   ```

7. `ConnectionMgr` removes the HTTP connection from its `_connections` registry and closes it.

8. The server releases the remaining `WebSocketState` after pending WebSocket output has completed.

```text
Browser closes socket
        |
        v
WebSocket async read returns EOF/reset
        |
        +--> WebSocketState::onError()
        |       |
        |       +--> application onError callback
        |       +--> remove WebSocketPtr from WebSocketContext
        |       +--> reset wsPtr
        |       +--> release wsContext
        |
        +--> processError()
                |
                +--> ConnectionMgr removes HTTP connection
                +--> HTTP connection closes
                +--> WebSocketState is eventually released
```

For a WebSocket close frame, the flow is similar, except `onClose()` is called instead of `onError()`.

```text
Browser sends close frame
        |
        v
WebSocket frame parser detects opcode 8
        |
        +--> send close response
        +--> WebSocketState::onClose()
        |       |
        |       +--> application onClose callback
        |       +--> remove WebSocketPtr from WebSocketContext
        |       +--> reset wsPtr
        |       +--> release wsContext
        |
        +--> processCompleted()
                |
                +--> ConnectionMgr removes HTTP connection
                +--> HTTP connection closes
```

`operation_aborted` is normally produced by a server-initiated cancellation. It is logged by `HttpConnection` and should not be relied on as the primary browser-disconnect cleanup path.

```text
WebSocket closure steps:
---------------------------------------------------
Browser closes socket
  -> WebSocket read returns EOF / connection_reset
  -> WebSocketState::onError(...)
       -> WebSocketContext::stop(wsPtr)
       -> _connections.erase(wsPtr)
       -> wsPtr.reset()
       -> wsContext.reset()
  -> processError(...)
  -> ConnectionMgr::handleOnComplete(...)
  -> ConnectionMgr::stop(connectionId)
       -> _connections.erase(httpConnection)
       -> connection->close()
```       