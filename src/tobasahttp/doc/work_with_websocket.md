# Work with WebSocket Connections in Tobasa HTTP

`WebSocketContext` is a shared registry and callback set for a group of WebSocket connections. It is not a socket. Each accepted connection has a separate `WebSocketPtr`; the context keeps those handles so the application can broadcast, find clients, and close connections.

## Create a context and accept an endpoint

Create the context once, install callbacks, and keep it alive while the endpoint is in use. The request handler assigns it to the HTTP context and returns `handled` with status `200 OK`:

```cpp
auto wsContext = std::make_shared<http::WebSocketContext>();

wsContext->onMessage = [](http::WebSocketPtr client, const std::string& message)
{
   client->sendText("echo: " + message);
};

serverHttp.requestHandler([wsContext](const http::HttpContext& context)
{
   if (context->request()->path() != "/websocket_ep")
      return http::RequestStatus::notHandled;

   context->webSocketContext(wsContext);
   context->response()->content("");
   context->response()->httpStatus(http::StatusCode::OK);
   return http::RequestStatus::handled;
});
```

For HTTP/1, the server calls the handler when it receives an `Upgrade: websocket` request. A `handled` result, `200 OK`, an attached context, and a non-empty `Sec-WebSocket-Key` let the server complete the handshake. It sends `101 Switching Protocols`, creates the connection handle, adds it to the context, calls `onOpen`, and starts reading frames. This path is implemented in [server_connection.h](../include/tobasahttp/server/server_connection.h).

HTTP/2 uses a separate extended-CONNECT stream path and does not send the HTTP/1 `101` response. It still requires the handler to return `handled`, set `200 OK`, and attach a context.

For HTTP/1, `notHandled` on an upgrade request produces `501 Not Implemented`. A handled request with `200 OK` but no context produces `500 Internal Server Error`; a missing key produces `426 Upgrade Required`. In the current HTTP/1 implementation, another handled, non-200 status is returned as `401 Unauthorized`, not as the status the handler set. Check access before accepting the upgrade, and account for this behavior when rejecting requests.

The handshake code has a special case for the `Bearer` WebSocket subprotocol: when it is the first offered protocol, the server echoes `Bearer`. Source comments describe placing a JWT in another offered protocol value as a non-standard convention. The current browser sample offers no subprotocol, so it does not use this behavior. Do not treat it as a general authentication API.

## Handle events and retain application state

The context exposes `onOpen`, `onMessage`, `onClose`, `onError`, `onPing`, and `onPong`. Set these callbacks before accepting connections. The server calls `onOpen` after it has written the handshake response. On close, it calls `onClose` before removing the handle from the context. On error, it calls `onError` and removes the handle; an error does not promise a separate `onClose` callback.

Callbacks may capture application state, but the context does not own objects captured by raw pointer or reference. Keep those objects alive for as long as callbacks may run. The sample server uses a long-lived wrapper for its context and captures that wrapper in callbacks; see [main.cpp](../../samples/http_server/src/main.cpp).

## Send messages and manage clients

The connection ID is assigned by the server. `identifier()` is an application-defined label; make it unique if your application uses it to find or address a client.

```cpp
wsContext->sendText("hello everyone");          // broadcast
wsContext->sendText("hello", connectionId);    // one connection
wsContext->sendText("private", "user-123");    // matching identifier

auto client = wsContext->findClient("user-123");
if (client)
   client->sendText("hello again");
```

For the connection-ID overload, `0` means broadcast. For the identifier overload, an empty identifier also means broadcast. `sendText()` and `sendBinary()` enqueue messages; they do not wait for the peer to receive them. Pass a send-error callback when the application needs to observe write failures.

`stop()` removes a client from the context's registry but does not close its socket. Use `close()` to remove and close a connection with a WebSocket close code and reason. When a peer disconnects or an error occurs, the server calls `onClose` or `onError` and removes that client automatically.

## Lifetime and concurrent access

The HTTP context keeps a shared pointer to the WebSocket context during upgrade, and the live WebSocket state keeps it while the connection is active. The application should still own the context while the endpoint is available so future requests can attach it.

`WebSocketPtr` is a handle, not ownership of the underlying HTTP connection. Internally it holds a weak connection reference. Keeping the handle after close does not keep the network connection alive; check `closed()` or `isClientConnected()` before acting on stale state.

`WebSocketContext` protects its connection set with a mutex and takes a snapshot before broadcast. That mutex does not protect your callback state or serialize sends from arbitrary application threads. Callbacks for different connections can run concurrently. The per-connection send queue has no application-facing synchronization; serialize sends for a connection on its execution context or add your own synchronization. Do not assume that every operation on the context or `WebSocketPtr` is thread-safe just because the connection set is protected.

## Serve a browser test page

The sample serves `/test_websocket` as a normal HTTP request, separately from `/websocket_ep`. `Response::fileContent()` makes a file the response body; it does not accept or upgrade a WebSocket connection. The sample checks the requested path and verifies that the resolved file stays under the document root before serving it:

```cpp
std::string documentRoot("./wwwroot");
std::string requestPath = context->request()->path();

if (requestPath.empty() || requestPath[0] != '/' ||
    requestPath.find("..") != std::string::npos)
{
   return statusResult(context, http::StatusCode::FORBIDDEN);
}

std::string fullPath = documentRoot + requestPath + ".html";

if (!path::isPathWithinRoot(fullPath, documentRoot))
   return statusResult(context, http::StatusCode::FORBIDDEN);

if (!path::exists(fullPath))
{
   return statusResult(context, http::StatusCode::NOT_FOUND);
}

context->response()->setHeaderContentType("text/html");
context->response()->httpStatus(http::StatusCode::OK);
context->response()->fileContent(fullPath);
return http::RequestStatus::handled;
```

This is only a test page route. Apply the same path validation when serving any request-derived file path. See the complete endpoint and page handlers in [main.cpp](../../samples/http_server/src/main.cpp), and the context and client APIs in [websocket.h](../include/tobasahttp/websocket.h).
