**SSE Cleanup Flow**

1. Browser closes the connection.

2. The pending SSE `async_write` completes with `EOF`, 
`connection_reset`, or another socket error.

3. The callback calls:

```cpp
removeSseConnection();
```

This:

```text
SseContext::_connections.erase(ssePtr)
SseState::ssePtr.reset()
```

4. The callback calls `processError()`.

5. `ConnectionMgr::handleOnError()` receives the error and calls:

```cpp
ConnectionMgr::stop(connectionId, reason);
```

6. `ConnectionMgr` removes the HTTP connection from its `_connections` registry and closes it.

7. The `ServerConnection` SSE state is reset:

```cpp
_sseState.reset();
```

The resulting ownership cleanup is:

```text
Browser
  closes socket
      |
      v
SSE async_write error
      |
      +--> remove from SseContext::_connections
      +--> reset SseState::ssePtr
      +--> processError()
              |
              +--> ConnectionMgr removes HTTP Connection
              +--> HTTP connection closes
              +--> ServerConnection releases _sseState
```

For an explicit application call to `SseConnection::close()`, the close handler performs the SSE registry cleanup, queues the terminating chunk, and `ConnectionMgr` closes the HTTP connection through `callClose()`.

One caveat remains: `operation_aborted` is only logged by `HttpConnection::processError()`. That is normally caused by a server-initiated close, but it should not be treated as the primary browser-disconnect cleanup path.