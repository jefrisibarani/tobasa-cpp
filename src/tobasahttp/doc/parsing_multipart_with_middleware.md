# Parse Multipart Uploads with Middleware

`MultipartMiddleware` lets Tobasa Web parse an upload while the HTTP body is still arriving. The middleware returns `RequestStatus::async`, then resumes the request chain after it has parsed the final multipart boundary. Your route handler sees the parsed body only after that work is complete.

## Select the middleware parser

The HTTP server parses multipart bodies internally by default. Turn that off before registering the middleware. With `Webapp`, the setting is `webapp.httpServer.enableMultipartParsing`; when it is `true`, `Webapp::useMultipart()` returns without adding `MultipartMiddleware`.

```json
{
   "webapp": {
      "httpServer": {
         "enableMultipartParsing": false,
         "temporaryDir": "./tmp"
      }
   }
}
```

Then register the middleware:

```cpp
webapp.useMultipart([](web::MultipartMiddlewareOption& option)
{
    option.temporaryDir = "./tmp";
});
```

The setting defaults to `true`. `MultipartMiddlewareOption::temporaryDir` controls where uploaded file parts are written. If it is empty, `MultipartParser` uses Tobasa's temporary directory. With a bare `HttpServer`, `http::Settings::enableMultipartParsing(false)` disables the internal parser but does not add this middleware; the application must provide its own body reader. See [settings.h](../include/tobasahttp/server/settings.h) and [multipart_middleware.h](../../tobasaweb/include/tobasaweb/multipart_middleware.h).

## Read fields and files in the handler

After the middleware has finished, a Tobasa Web route handler can read the parsed body from the request:

```cpp
http::ResultPtr UploadController::onUpload(const web::RouteArgument& arg)
{
   auto request = arg.httpContext()->request();
   if (!request->hasMultipartBody())
      return http::statusResultHtml(http::StatusCode::BAD_REQUEST);

   auto body = request->multipartBody();
   auto title = body->value("title");
   auto upload = body->find("file");
   if (!upload || !upload->isFile)
      return http::statusResultHtml(http::StatusCode::BAD_REQUEST, "Missing file part");

   // Copy upload->location to application-owned storage before returning.
   return http::statusResultHtml(http::StatusCode::OK, "Upload saved");
}
```

This follows the app server's upload handler in [api_users_controller.cpp](../../app_server/src/core/api_users_controller.cpp). `MultipartBody::value(name)` returns the first matching part's in-memory `body`. It returns an empty string when no part matches, when a field is empty, and for file parts whose bytes are stored on disk. Use `find(name)` or iterate `parts()` when presence or repeated field names matter. File parts have `isFile`, the client-supplied `fileName`, and a temporary-file `location`; do not use `fileName` as a destination path.

## What happens while the body arrives

When internal parsing is disabled, `ServerConnection` gives the request a `MultipartBodyReader` and starts the application pipeline before the body is complete. `MultipartMiddleware` reads any bytes already buffered, then the reader requests more data as the socket receives it.

For a fixed-length body, the reader passes received body bytes to `MultipartParser`. For a chunked body, the HTTP parser removes HTTP chunk framing first; the multipart parser receives only the chunk payload. It keeps enough trailing bytes between reads to find a multipart delimiter split across buffers. File parts are streamed to temporary files, while ordinary field data is collected in memory.

Because parsing is not finished when `invoke()` returns, the middleware returns `RequestStatus::async`. When it finds the closing multipart boundary, it moves the `MultipartBody` onto the request, marks the reader done, invokes the next middleware or route, and completes the HTTP context with that handler's status. The server sends the response only after completion.

## Errors and temporary-file lifetime

The middleware throws if a multipart request has no `MultipartBodyReader` or if its boundary is absent or rejected. Parser errors are returned to `ServerConnection`, which handles them as request errors; the normal downstream handler is not called for a failed parse. A file-part open failure also throws. Make sure the temporary directory exists or can be created and is writable by the server.

The parser writes file data to a generated temporary path and stores that path in `Part::location`. The request cleanup removes these files when request processing completes. Copy or move a file to application-owned storage while handling the request; do not queue `location` for later work after returning the response. The original `fileName` comes from the client and is untrusted. The app server's [profile upload handler](../../app_server/src/core/api_users_controller.cpp) copies the temporary file before request cleanup.

`hasMultipartBody()` returns false for a null or empty body. `MultipartParser` checks the body against `Content-Length` when present and reports a missing closing boundary or excess data as a parse error. Its boundary validator is limited: it rejects an empty boundary, a value longer than 70 characters, and a trailing space, but it accepts as soon as it finds one allowed character rather than checking every character. Do not treat it as a full validation of untrusted upload content.

The middleware and reader implementations are in [multipart_middleware.cpp](../../tobasaweb/src/tobasaweb/multipart_middleware.cpp), [multipart_body_reader.cpp](../src/tobasahttp/multipart_body_reader.cpp), and [multipart_parser.cpp](../src/tobasahttp/multipart_parser.cpp). For the server-owned parsing path, see [parsing_multipart_internally.md](parsing_multipart_internally.md).
