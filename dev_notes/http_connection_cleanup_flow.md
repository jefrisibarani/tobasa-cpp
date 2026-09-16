# HTTP Connection Cleanup Flow

## Browser Closes the Connection

1. The browser closes the TCP connection.

2. The pending server read or write operation completes with an error such as:

   - `asio::error::eof`
   - `asio::error::connection_reset`

3. `ServerConnection` calls `HttpConnection::processError()`.

4. `processError()` treats `EOF` and `connection_reset` as normal remote
   completion and invokes the registered `_onComplete` handler.

5. `ConnectionMgr::handleOnComplete()` receives the connection ID and calls:

   ```cpp
   ConnectionMgr::stop(connectionId, reason);
   ```

6. `ConnectionMgr::stop()` locks `_connectionsMutex`, finds the matching
   `ConnectionPtr`, removes it from `_connections`, and releases the mutex.

7. After unlocking, `ConnectionMgr::stop()` calls `target->close()`.

8. `HttpConnection::close()` cancels timers, shuts down the socket, closes the
   socket, marks the connection closed, and invokes the registered `onClosed`
   handler.

9. The final `ConnectionPtr` is released after the asynchronous handlers and
   manager references are gone, allowing `ServerConnection` to be destroyed.

```text
Browser closes TCP connection
          |
          v
Read/write completes with EOF or connection_reset
          |
          v
ServerConnection::processError()
          |
          v
ConnectionMgr::handleOnComplete()
          |
          v
ConnectionMgr::stop(connectionId)
          |
          +--> erase HTTP ConnectionPtr from ConnectionMgr::_connections
          |
          v
target->close()
          |
          +--> cancel timers
          +--> shutdown socket
          +--> close socket
          +--> mark connection closed
          +--> invoke onClosed
```

## Server-Initiated Close

When the server closes the connection first, `ConnectionMgr::stop()` removes
the connection from its registry before calling `target->close()`. The socket
operation cancelled by `close()` may later complete with
`asio::error::operation_aborted`.

`HttpConnection::processError()` only logs `operation_aborted`; it does not
invoke `_onComplete` again. This avoids duplicate cleanup because the manager
already removed the connection.