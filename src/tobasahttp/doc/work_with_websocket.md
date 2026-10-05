# Working with WebSocket in Tobasa

This note explains what `WebSocketContext` is and how to use it to handle WebSocket clients in a simple way.

## 1. What is `WebSocketContext`?

`WebSocketContext` is the server-side manager for one WebSocket endpoint.

It is not a single socket. It is a container for a group of live WebSocket connections that belong to the same endpoint or the same feature area. For example, all clients connected to `/chat` can be tracked by one `WebSocketContext`, and all clients connected to `/alerts` can be tracked by another one.

Its job is to:

- keep a list of connected clients
- call your event handlers when clients connect, send messages, or disconnect
- send data to one client or to many clients
- help you find a client by its id or custom identifier
- close or stop a client connection when needed

Common callback hooks are:

- `onOpen`: called when a client connects
- `onClose`: called when a client disconnects
- `onMessage`: called when a client sends a message
- `onError`: called when something goes wrong
- `onPing`: called when a ping frame arrives
- `onPong`: called when a pong frame arrives

In short, `WebSocketContext` is the place where the server keeps the state and behavior of a WebSocket feature.

## 2. More about what it manages

A `WebSocketContext` holds a set of connection objects, each represented by `http::WebSocketPtr`.

This means one context can manage many clients at the same time. That is why it is useful for features like:

- chat rooms
- live dashboards
- push notifications
- admin monitoring
- event broadcasting

The most important methods are:

- `addConnection(...)`: register a new client connection
- `stop(...)`: stop one connection
- `close(...)`: close one connection with a close code and reason
- `findClient(...)`: look up a client by identifier
- `isClientConnected(...)`: check whether the client is still connected
- `hasClient()`: check whether there is at least one client connected
- `sendText(...)`: send text to one client or all clients
- `sendBinary(...)`: send binary data to one client or all clients

The class is designed to be used as a manager for an endpoint, not as a low-level socket object.

## 3. How it is attached to an endpoint

The request handler decides which `WebSocketContext` belongs to which endpoint.

This is the important rule:

- the URL decides which context is used
- the request handler assigns the context
- the server upgrades the HTTP connection to WebSocket

Example:

```cpp
auto chatWs = std::make_shared<http::WebSocketContext>();
auto alertWs = std::make_shared<http::WebSocketContext>();

serverHttp.requestHandler(
   [&](const http::HttpContext& context)
   {
      if (context->request()->path() == "/chat")
      {
         context->webSocketContext(chatWs);
         context->response()->httpStatus(http::StatusCode::OK);
         return http::RequestStatus::handled;
      }

      if (context->request()->path() == "/alerts")
      {
         context->webSocketContext(alertWs);
         context->response()->httpStatus(http::StatusCode::OK);
         return http::RequestStatus::handled;
      }

      return http::RequestStatus::notHandled;
   });
```

That means you can create many WebSocket contexts and map them to many different endpoints.

## 4. How WebSocket upgrade works

The normal flow is:

1. A client sends an HTTP request to a server endpoint.
2. The server decides whether to accept the upgrade.
3. If accepted, the request handler sets a `WebSocketContext` on the current HTTP context.
4. The server upgrades the connection from HTTP to WebSocket.
5. The WebSocket client and server can then send messages to each other.

In Tobasa, this is done in the request handler with code like this:

```cpp
wsContext = std::make_shared<http::WebSocketContext>();

context->webSocketContext(wsContext);
response->content("");
response->httpStatus(http::StatusCode::OK);
return http::RequestStatus::handled;
```

The important part is this:

- the request handler must accept the HTTP upgrade
- it must assign a `WebSocketContext` to the current HTTP context
- then the connection becomes a real WebSocket connection

## 3. A simple WebSocketContext setup

This is a basic pattern:

```cpp
auto wsContext = std::make_shared<http::WebSocketContext>();

wsContext->onOpen = [](http::WebSocketPtr conn)
{
   std::cout << "Client connected: " << conn->id() << std::endl;
};

wsContext->onMessage = [](http::WebSocketPtr conn, const std::string& message)
{
   std::cout << "Message from client: " << message << std::endl;
   conn->sendText("echo: " + message);
};

wsContext->onClose = [](http::WebSocketPtr conn, int closeCode, const std::string& reason)
{
   std::cout << "Client closed: " << conn->id() << std::endl;
};

wsContext->onError = [](http::WebSocketPtr conn, const http::ErrorData& error)
{
   std::cout << "WebSocket error: " << error.message << std::endl;
};
```

After that, the request handler can assign this context to the HTTP request:

```cpp
context->webSocketContext(wsContext);
```

This tells the server: "this upgraded connection belongs to this WebSocketContext".

## 4. How to work with connected clients

`WebSocketContext` keeps a list of connected clients.

You can:

- send text to a single client
- send binary data to a single client
- send text to all clients
- find a client by identifier
- check if a client is still connected
- close a specific client connection

### Send to one client

```cpp
wsContext->sendText("Hello client", connId);
```

### Send to all clients

```cpp
wsContext->sendText("Broadcast message");
```

### Send to a client by custom identifier

```cpp
wsContext->sendText("Private message", "client-123");
```

### Find a client

```cpp
auto client = wsContext->findClient("client-123");
if (client)
{
   client->sendText("Hi there");
}
```

## 5. What the client object gives you

Each connected client is represented by `http::WebSocketPtr`.

This object gives you access to:

- `id()`: unique connection id
- `identifier()`: a custom label for the client
- `remoteEndpoint()`: client IP and port
- `userData()`: custom data attached to the connection
- `sendText(...)`: send a text message
- `sendBinary(...)`: send binary data
- `close(...)`: close the connection

Example:

```cpp
conn->identifier("user_123");
conn->sendText("Welcome to the chat");
```

## 6. Event-driven model

The WebSocket pattern in Tobasa is event-based.

The server waits for events and reacts to them:

- client connects -> `onOpen`
- client sends data -> `onMessage`
- client disconnects -> `onClose`
- client sends ping -> `onPing`
- client sends pong -> `onPong`
- protocol error -> `onError`

This is a common pattern for chat apps, push updates, realtime dashboards, and live status streams.

## 7. Example from the sample server

The sample server in `src/samples/http_server/src/main.cpp` shows the full pattern.

### 7.1 Create the WebSocketContext

The sample creates a wrapper object:

```cpp
class WebSocketCtxWrapper 
{
public:
   WebSocketCtxWrapper() 
   {
      createWebSocketContext();
   }

   std::shared_ptr<tbs::http::WebSocketContext> wsContext;

   void createWebSocketContext();
};
```

Then it builds the actual context and attaches all handlers:

```cpp
void WebSocketCtxWrapper::createWebSocketContext()
{
   wsContext = std::make_shared<http::WebSocketContext>();

   wsContext->onOpen = [this](http::WebSocketPtr conn)
   {
      std::cout << "[websocket:" << conn->id() << "] connection started";
      conn->identifier(util::getRandomString(6));

      std::ostringstream out;
      out << "Welcome to Tobasa Web Socket Service" << std::endl;
      out << "Your connection ID: " << conn->id() << std::endl;
      conn->sendText(out.str());
   };

   wsContext->onMessage = [this](http::WebSocketPtr conn, const std::string& message)
   {
      std::cout << "[websocket:" << conn->id() << "] received data: " << message << std::endl;

      if (util::startsWith(message, "MESSAGE|"))
      {
         auto items = util::split(message, "|");
         if (items.size() == 3)
         {
            auto destination = items[1];
            if (util::isNumber(destination))
            {
               auto destinationId = std::stoll(destination);
               wsContext->sendText("MESSAGE from ID " + std::to_string(conn->id()) + " : " + items[2], destinationId);
            }
            else if (destination == "ALL")
            {
               wsContext->sendText("MESSAGE from ID " + std::to_string(conn->id()) + " : " + items[2]);
            }
            else
            {
               conn->sendText("Invalid MESSAGE destination syntax");
            }
         }
         else
         {
            conn->sendText("Invalid MESSAGE syntax");
         }
      }
      else
      {
         conn->sendText("[echo] " + message);
      }
   };

   wsContext->onClose = [](http::WebSocketPtr conn, int closeCode, const std::string& reason)
   {
      std::cout << "[websocket:" << conn->id() << "] connection closed" << std::endl;
   };

   wsContext->onError = [](http::WebSocketPtr conn, const http::ErrorData& error)
   {
      std::cout << "[websocket:" << conn->id() << "] Error code: " << error.code << " " << error.message << std::endl;
   };
}
```

This is the main pattern for working with WebSocket clients:

- create the context
- register callbacks
- handle messages
- send replies or broadcasts

### 7.2 Accept the upgrade request

The sample creates a WebSocket endpoint at `/websocket_ep`:

```cpp
tbs::http::RequestStatus handleWebsocketEndpoint(const tbs::http::HttpContext& context)
{
   auto response = context->response();
   auto wsContext = wsCtxWrapper->wsContext;

   context->webSocketContext(wsContext);

   response->content("");
   response->httpStatus(http::StatusCode::OK);

   return http::RequestStatus::handled;
}
```

This means:

- the HTTP request is accepted as a WebSocket upgrade
- the server binds the request to the prepared `WebSocketContext`
- the WebSocket handshake is then completed by the server

### 7.3 Serving a client page

The sample also handles `/test_websocket` by serving an HTML page to the browser:

```cpp
tbs::http::RequestStatus handleWebsocket(const tbs::http::HttpContext& context)
{
   auto response = context->response();
   std::string fullPath = "./wwwroot" + requestPath + ".html";

   response->setHeaderContentType(http::mimetypes::fromExtension("html"));
   response->httpStatus(http::StatusCode::OK);
   response->fileContent(fullPath);

   return http::RequestStatus::handled;
}
```

This page is used for the browser-side WebSocket client.

### 7.4 Message handling in the sample

The sample supports a simple message format:

```text
MESSAGE|{destination}|{data}
```

Examples:

- `MESSAGE|ALL|hello everyone`
- `MESSAGE|12|hello client 12`

When the server receives that message, it either:

- sends it to one client by id
- sends it to all connected clients
- or returns a syntax error

If the message does not start with `MESSAGE|`, the sample simply echoes it back:

```cpp
conn->sendText("[echo] " + message);
```

This example is useful because it shows the normal WebSocket server pattern: receive message, inspect it, decide what to do, then send a response.

## 8. Summary

`WebSocketContext` is the main object used to manage a WebSocket endpoint in Tobasa.

It lets you:

- accept upgraded WebSocket connections
- track connected clients
- handle open, close, message, and error events
- send messages to one client or many clients
- build small realtime apps with very little code

The sample in `src/samples/http_server/src/main.cpp` is a good reference for a real implementation. It shows how to:

- create a `WebSocketContext`
- assign it in a request handler
- handle connection events
- process incoming messages
- send replies and broadcasts

This is the basic pattern to follow when building a WebSocket service in Tobasa.
