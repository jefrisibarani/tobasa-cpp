# HTTP Server Sample

A small HTTP/HTTPS server example using the Tobasa HTTP library.

## Overview

This sample creates one plain HTTP server and one HTTPS server. Both servers
use the same request handler, the same `wwwroot` directory, and the same
WebSocket context.

The sample is useful when you want to see how to:

- create HTTP and HTTPS listeners;
- configure request timeouts, buffers, multipart parsing, and connection
  limits;
- serve files from a web root;
- handle a request directly with `HttpContext` and `Response`;
- receive a multipart file upload;
- upgrade a request to WebSocket handling.

## Features

- HTTP/1.1 support;
- optional HTTP/2 support on HTTPS when `TOBASA_HTTP_USE_HTTP2` is enabled;
- plain HTTP on port `8084`;
- HTTPS on port `8085`;
- static files from `wwwroot`;
- uploaded files served from `data` through `/uploads/`;
- multipart upload handling;
- WebSocket endpoint and browser test page;
- request and connection logging.

## Building

The sample is built as part of the main build system. The executable is
written to:

```text
_output/http_server/debug/
```

The post-build step copies these resources beside the executable:

- `wwwroot/` from the sample directory;
- TLS files from the sample `resources/` directory;
- timezone data when it is not embedded in the build.

## Running

```bash
./http_server
```

The server listens on all interfaces:

- HTTP: `http://localhost:8084`
- HTTPS: `https://localhost:8085`

The HTTPS certificate is intended for local testing. With `curl`, use `-k` to
skip certificate verification:

```bash
curl http://localhost:8084/
curl -k https://localhost:8085/
```

Press `Ctrl+C` to stop both servers. The application stops the listeners and
releases the WebSocket context before stopping the I/O context.

## How requests are handled

`main.cpp` creates one `asio::io_context` and starts an I/O thread pool with
four threads. Both `PlainServerDefault` and `SecureServerDefault` register the
same function:

```cpp
serverHttp.requestHandler(
   [&](const http::HttpContext& context) {
      return handleServerRequest(context);
   });
```

`handleServerRequest()` checks the request path:

1. `/hello` goes to `handleHelloPage()`.
2. `/upload` goes to `handleUpload()`.
3. `/websocket_ep` prepares the WebSocket upgrade.
4. `/test_websocket` serves the WebSocket browser page.
5. Any other path goes to `handleIndexPage()` and is read from disk.

Each handler fills `context->response()` and returns
`RequestStatus::handled`. The server then sends the response. This sample does
not use a separate controller or router object; the path selection is done in
`handleServerRequest()`.

## Exposed endpoints

### `/hello` (normally `GET`)

Returns a small HTML page containing `Hello World!`.

It also demonstrates response features:

- `X-Processed-By: Request Handler`;
- `Content-Type: text/html`;
- a cookie named `cookie_test_1`;
- removal of `cookie_test_2`;
- a session cookie named `cookie_test_3`.

### `/upload` (normally `POST`)

Handles a request body in two ways:

- for a multipart request containing a file part named `profileImage`, it
  returns that uploaded file to the client;
- otherwise, it returns the request body as an HTML page.

Multipart parsing is enabled in the server settings. Uploaded temporary files
are stored under `./tmp`.

Example multipart request:

```bash
curl -F "profileImage=@image.png" http://localhost:8084/upload
```

The response uses the uploaded part's content type and sends the file with
chunked encoding.

### `/test_websocket` (normally `GET`)

Serves `wwwroot/test_websocket.html`. This is the browser page used to test
the WebSocket endpoint.

The handler safely checks the path and then looks for this file:

```text
./wwwroot/test_websocket.html
```

If the path is invalid or the file is missing, it returns `403 Forbidden` or
`404 Not Found`.

### `/websocket_ep` (normally `GET` with an upgrade request)

This is the WebSocket endpoint. The handler attaches a shared
`WebSocketContext` to the HTTP context and returns a handled response. The
HTTP server then performs the WebSocket upgrade.

When a client connects, the WebSocket context:

1. assigns a random identifier;
2. sends a welcome message with the connection ID and user ID;
3. explains the message format.

Messages use this format:

```text
MESSAGE|{destination}|{data}
```

`{destination}` can be a numeric connection ID or `ALL`.

Examples:

```text
MESSAGE|12|hello connection 12
MESSAGE|ALL|hello everyone
```

Numeric destinations receive a message addressed to that connection. `ALL`
broadcasts to every connected client. An invalid message or destination is
returned to the sender as an error message.

Messages that do not start with `MESSAGE|` are echoed back with an `[echo]`
prefix.

The sample also logs open, close, ping, pong, message, and error events.

## `wwwroot` files

Paths that are not one of the special endpoints are looked up under
`./wwwroot`. A request ending in `/` gets `index.html` appended. The sample
dispatches by path and does not enforce `GET` or `POST` in
`handleServerRequest()`, so the methods shown above describe the intended use.

The sample web root contains:

| Path | Use |
| --- | --- |
| `/` or `/index.html` | Main sample page. |
| `/login.html` | Login page example. |
| `/register.html` | Registration page example. |
| `/password.html` | Password page example. |
| `/test_websocket.html` | WebSocket test client. |
| `/demo.json` | Static JSON example. |
| `/css/...` | CSS files. |
| `/js/...` | JavaScript files. |
| `/assets/...` | Images and other application assets. |
| `/vendor/...` | Third-party browser libraries. |

The handler checks that the request path starts with `/` and does not contain
`..`. Unsafe paths return `403 Forbidden`. Missing files return `404 Not Found`.

For normal paths, the response MIME type is selected from the file extension
and the file is sent with `response->fileContent()`.

### `/uploads/` mapping

Requests beginning with `/uploads/` are read from `./data` instead of
`./wwwroot`:

```text
/uploads/report.pdf  ->  ./data/uploads/report.pdf
```

The same path validation and missing-file checks apply. Make sure the `data`
directory and requested file exist before testing this path.

## Server settings used by the sample

The sample configures these important values:

| Setting | HTTP | HTTPS |
| --- | --- | --- |
| Address | `0.0.0.0` | `0.0.0.0` |
| Port | `8084` | `8085` |
| Read timeout | `10` seconds | `10` seconds |
| Write timeout | `60` seconds | `60` seconds |
| Processing timeout | `3600` seconds | `3600` seconds |
| Read/send buffers | `32 KB` | `32 KB` |
| Maximum header size | `1 MB` | `1 MB` |
| Multipart parsing | Enabled | Enabled |
| Temporary directory | `./tmp` | `./tmp` |

HTTPS uses `localhost.crt`, `localhost.key`, and `dh2048.pem`. HTTP/2 is
enabled on HTTPS only when the source is compiled with
`TOBASA_HTTP_USE_HTTP2`.

## Source files

- [`src/main.cpp`](src/main.cpp) contains the server setup, request handler,
  static-file handling, upload handling, and WebSocket handling.
- [`cmake/sources.cmake`](cmake/sources.cmake) lists the source files used by
  the target.
- [`CMakeLists.txt`](CMakeLists.txt) copies `wwwroot`, TLS files, and optional
  timezone data beside the executable.

## License

See [LICENSE](../../../LICENSE) in the repository root.
