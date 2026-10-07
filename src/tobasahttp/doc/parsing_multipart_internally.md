# Parse Multipart Requests Inside the HTTP Server

With internal parsing enabled, the HTTP parser consumes the whole request body before it calls your request handler. It gives the handler a parsed `MultipartBody`: ordinary fields are in memory, and uploaded file contents are in temporary files.

## Enable internal parsing

`enableMultipartParsing` is `true` by default. The handler can read the parsed body from the request:

```cpp
auto body = context->request()->multipartBody();
if (!body)
{
   context->response()->httpStatus(http::StatusCode::BAD_REQUEST);
   return http::RequestStatus::handled;
}

auto title = body->value("title");
auto upload = body->find("file");
if (upload && upload->isFile)
{
   // Use upload->location while this request is active.
}
```

For a plain `HttpServer`, internal parsing is enabled by default and can be selected explicitly through `http::Settings`:

```cpp
http::Settings settings;
settings.enableMultipartParsing(true);
```

The same setting is available in Webapp configuration at `webapp.httpServer.enableMultipartParsing`. When it is `false`, the server does not use this internal path. A Webapp can instead install `MultipartMiddleware`; see [parsing_multipart_with_middleware.md](parsing_multipart_with_middleware.md).

## What the server does

The HTTP parser checks `Content-Type` for `multipart/form-data`, reads its boundary, and selects the body framing from `Content-Length` or `Transfer-Encoding: chunked`. It feeds arriving body data to `MultipartParser` as socket reads complete; it does not buffer the entire upload in memory first.

HTTP chunk framing and multipart boundaries are separate layers. For chunked requests, the HTTP parser removes each chunk's size and framing before the multipart parser sees the payload. The multipart parser keeps a short tail between reads so it can recognize a boundary split across buffers. For both body modes, the server waits for the final multipart boundary; with chunked transfer, it also finishes reading the final HTTP chunk and trailers before calling the handler.

After the body is complete, the server moves the parsed body onto the request and calls the normal request handler. If parsing fails, the handler is not called. The parser result goes to `ServerConnection::handleRequestError()`; an error without a specific HTTP status becomes `400 Bad Request`. Errors with an explicit status keep that status.

## Read fields and manage uploaded files

`MultipartBody::value(name)` returns the first matching part's in-memory body. It returns an empty string when the part is absent, empty, or a file part, so use `find(name)` when presence or part type matters. `find()` also returns only the first matching part; use `parts()` if repeated field names are meaningful.

For a file part, `fileName` is supplied by the client and must not be trusted as a filesystem path. `location` is the server-generated temporary file path. Copy the file to an application-owned destination while handling the request. At request completion, the server calls `MultipartBody::cleanup(true)`: it deletes temporary files and clears the parts. Keeping a `MultipartBodyPtr` does not keep its parts or uploaded files available after cleanup. The app server follows this pattern in [api_users_controller.cpp](../../app_server/src/core/api_users_controller.cpp).

The temporary directory comes from `http::Settings::temporaryDir`; its default is `./tmp`. Ensure the server process can create and write files there. Field values are held in memory, while file bytes are streamed to disk, so large regular fields can still consume memory.

## Boundaries and malformed bodies

The `Content-Type` header must include a valid multipart boundary. The current validator rejects an empty boundary, one longer than 70 characters, or one ending in a space. It accepts the boundary after finding an allowed character rather than validating every character, so do not treat it as a complete validation of untrusted input.

When `Content-Length` is present, the parser checks that the received body length agrees with it and that the closing boundary is present. Missing closing boundaries and excess body data are parse errors. File creation failures also fail parsing. In these cases, the server sends an error response and closes the connection instead of calling the handler.

The setting is declared in [settings.h](../include/tobasahttp/settings.h). The parser path is implemented in [http_parser.cpp](../src/tobasahttp/http_parser.cpp), [multipart_parser.cpp](../src/tobasahttp/multipart_parser.cpp), and [server_connection.h](../include/tobasahttp/server/server_connection.h).
