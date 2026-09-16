# HTTP Response and Result

The short version is:

- `http::Response` is the actual HTTP response sent to the client.
- `http::Result` is what controller code normally returns. The server converts
  it into a `Response` before sending it.

If you are writing a low-level HTTP handler, use `Response`. If you are writing
a controller route, return a `Result`.

## 1. What `http::Response` is

`http::Response` is defined in:

- `src/tobasahttp/include/tobasahttp/response.h`
- `src/tobasahttp/src/tobasahttp/response.cpp`

This is the object the server uses to write an HTTP answer to the socket.

It contains:

- HTTP status code;
- headers;
- body data;
- content type;
- a file or binary content source;
- gzip/compression state;
- redirect information.

The server eventually serializes this object before sending it:

```cpp
ResponseSerializer serializer(_httpContext->response());
serializer.serializeHttp1(&this->_sendBuffer, this->_settings.sendBufferSize());
```

So `Response` is the last object changed before the response becomes bytes on
the network.

### Common things you can do with `Response`

```cpp
auto response = ctx->response();
response->httpStatus(tbs::http::StatusCode::OK);
response->content("hello from Tobasa");
response->setHeaderContentType("text/plain");
```

You can also send a redirect:

```cpp
ctx->response()->redirect("/login");
```

Or send a file:

```cpp
ctx->response()->fileContent("/path/to/file.html");
ctx->response()->setHeaderContentType("text/html");
```

The body can be:

- normal text;
- a file on disk;
- raw binary data.

`Response::content()`, `Response::fileContent()`, and
`Response::rawBytesContent()` are the main methods for these body types.

## 2. Why `Response` is not enough for controllers

In a controller, it is usually simpler to return a typed result than to build
the response yourself.

This project has `web::ApiResult`, which inherits from `http::Result`. Its
source is here: [src/app_server/src/api_result.h](../src/app_server/src/api_result.h).

Example:

```cpp
//! Handle GET request to /api/users/{user_id:int}/roles
http::ResultPtr ApiUsersController::onRoles(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto parId = arg.get("user_id");
   if (!parId)
      return web::badParameter("Invalid value for parameter user id");

   if (!util::isNumber(parId.value()))
      return web::badParameter("Invalid value for parameter user id");

   auto authDbRepo = _dbService->createAuthDbRepo();

   long userId = std::stol(parId.value());
   if (!authDbRepo->exists(userId))
      return web::notFound("User not found");

   auto userRoles = authDbRepo->getUserRoleDto(userId);
   return web::object(userRoles);
}
```

The controller only describes the result it wants to return. The web layer
handles the status, headers, and body conversion.

That is the main reason `http::Result` exists.

## 3. What `http::Result` is

`http::Result` is defined in:

- `src/tobasaweb/include/tobasaweb/result.h`
- `src/tobasaweb/src/tobasaweb/result.cpp`

It is a higher-level description of the answer from a route. It can contain
the status, content, content type, redirect path, and other response data.

Common result classes include:

- `http::StatusResult`;
- `http::FileResult`;
- `http::RawBytesResult`.

A controller normally returns one of these, often through a helper function:

```cpp
return http::fileResult(fullPath);
```

or:

```cpp
return http::rawBytesResult(bytes, "application/octet-stream");
```

or:

```cpp
return http::statusResultHtml(StatusCode::BAD_REQUEST, "Invalid input");
```

## 4. How `Result` becomes a real HTTP response

The router performs the conversion.

Inside `Router::invoke()`, the flow is similar to this:

```cpp
auto result = entry.invokeHandler(std::move(routeArgument));
result->toResponse(response);
```

`Result::toResponse()` copies the result values into a real `Response`:

```cpp
void Result::toResponse(std::shared_ptr<Response> response, ResultContentBuilder contentBuilder)
{
   response->httpStatus(this->httpStatus());

   if (redirected()) {
      response->redirect(_redirectPath);
      return;
   }

   response->content(std::move(_content));
   response->setHeaderContentType(this->contentType());
}
```

The complete flow is:

```text
controller returns Result
    -> router calls Result::toResponse()
    -> Response receives the status, headers, and body
    -> server serializes Response and sends it to the client
```

## 5. Typical result types

### `StatusResult`

Use this for an error page or a simple response with an HTTP status:

```cpp
auto result = std::make_shared<tbs::http::StatusResult>(
   tbs::http::StatusCode::OK,
   "Welcome"
);
return result;
```

### `FileResult`

Use this for a static file or a download:

```cpp
return tbs::http::fileResult(filePath);
```

The response reads the file directly instead of first building one large
string in memory.

### `RawBytesResult`

Use this for binary data such as an image or generated file:

```cpp
return tbs::http::rawBytesResult(data, "image/png");
```

## 6. Which one should you use?

Use `Response` when:

- you are writing a low-level request handler;
- you need direct control over headers and body data;
- you are implementing custom protocol behavior.

Use `Result` when:

- you are writing a controller or route handler;
- you want a clean, typed return value;
- you want the library to copy the status, headers, and body for you.

## 7. Real example from this project

This is the low-level handler pattern used in Tobasa:

```cpp
auto handler = [](const tbs::http::HttpContext& ctx) -> tbs::http::RequestStatus
{
   auto response = ctx->response();
   response->httpStatus(tbs::http::StatusCode::OK);
   response->content("hello");
   response->setHeaderContentType("text/plain");
   return tbs::http::RequestStatus::handled;
};
```

This is the controller pattern:

```cpp
return http::statusResultHtml(StatusCode::NOT_FOUND, "page not found");
```

Both patterns are valid. They work at different levels of the HTTP stack.

## 8. Simple rule

Use `Response` when you are close to the HTTP transport. Use `Result` when you
are writing application or controller code.

The server always ends with a `Response`, but application code normally starts
with a `Result`.

`http::Result` is the simple typed way for application code to describe an
answer. `http::Response` is the final HTTP object written to the client.
