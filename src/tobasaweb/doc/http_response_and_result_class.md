# Return a Result or Build a Response

In a Tobasa Web route, return an `http::ResultPtr`. The router turns that description into the `http::Response` that the HTTP server sends. Use `Response` directly only when you are writing a lower-level HTTP handler.

## Return a result from a route

For example, return a status page for invalid input or a file result for a download:

```cpp
if (!validRequest)
   return http::statusResultHtml(http::StatusCode::BAD_REQUEST, "Invalid input");

return http::fileResult(filePath);
```

The route handler type returns a shared pointer to `http::Result`, so return a non-null result on every path. The router dereferences the pointer to inspect its class and convert it. A null result is not treated as “not found”; it will fail during dispatch.

Use `http::rawBytesResult()` when the body is binary data:

```cpp
return http::rawBytesResult(imageBytes, "image/png");
```

`imageBytes` must remain valid until the HTTP server finishes sending the response. `RawBytesResult` stores a non-owning span, and `BytesReader` does not copy or own the supplied bytes. Do not return a span over a local vector. Keep the storage alive through transmission. See [result.h](../include/tobasaweb/result.h) and [bytes_reader.h](../../tobasa/include/tobasa/bytes_reader.h).

Tobasa Web applications can define result classes for their own formats. The app server's `web::ApiResult`, for example, derives from `http::Result`; see [api_result.h](../../app_server/src/api_result.h).

## How the router builds the response

The router calls `Result::toResponse()` before returning from the route handler. The result supplies the status, content type, optional content disposition, and body. A registered content builder can build the body from the result. Redirect results set the redirect and skip normal body conversion.

`FileResult` creates a file reader for its path rather than copying the file contents into a string. `RawBytesResult` creates a byte reader over its borrowed span. The response and its data reader are retained while the server sends the data; any memory referenced by a byte span must outlive that send. The implementation is in [result.cpp](../src/tobasaweb/result.cpp) and the result types are declared in [result.h](../include/tobasaweb/result.h).

## Use Response in a low-level handler

A low-level `RequestHandler` writes directly to `context->response()` and returns a `RequestStatus`:

```cpp
auto handler = [](const http::HttpContext& context)
   -> http::RequestStatus
{
   auto response = context->response();
   response->httpStatus(http::StatusCode::OK);
   response->content("hello");
   response->setHeaderContentType("text/plain");
   return http::RequestStatus::handled;
};
```

Use this form when you need direct access to response headers, body sources, or transport behavior. `RequestStatus::handled` means the response is ready. For asynchronous work, follow the context completion contract instead of returning `handled` before the response is ready. For controller routes, prefer a result. The response API is declared in [response.h](../../tobasahttp/include/tobasahttp/response.h).
