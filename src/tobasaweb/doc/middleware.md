# Middleware in Tobasa Web

Middleware is an ordered chain around the router. Each step sees the request before the next step. A step either calls `next(context)` or finishes the request itself.

## Add a simple check

Pass a callback to `Webapp::addMiddleware()` when a small function is enough:

```cpp
webapp.addMiddleware(
   [](const http::HttpContext& context, const http::RequestHandler& next)
   {
      if (shouldReject(context))
      {
         auto result = http::statusResultHtml(
            http::StatusCode::FORBIDDEN, "Request rejected");
         result->toResponse(context->response());
         return http::RequestStatus::handled;
      }

      return next(context);
   },
   "RequestCheck");
```

The callback returns the status from `next(context)` or `RequestStatus::handled` after it has written a response. The overload wraps it in `AutoMiddleware`; see [webapp.cpp](../src/tobasaweb/webapp.cpp) and [middleware.h](../include/tobasaweb/middleware.h).

## Continue or finish the request

Use `next(context)` when later middleware and the router should run. Before returning `RequestStatus::handled`, write the response; the chain will not call a later handler to do it. Middleware receives a const reference to the context, but the context is a shared object, so its request data or response can be updated through the context API.

`WebService::setupHandlers()` appends middleware in registration order and then appends the router. `MiddlewareManager` retains each middleware in a `shared_ptr` and connects each step to the next one. The router prepares route and authentication information before the first middleware runs, but route dispatch happens at the end of the chain. See [web_service.cpp](../src/tobasaweb/web_service.cpp) and [middleware_manager.cpp](../src/tobasaweb/middleware_manager.cpp).

The manager invokes a middleware object for each request; it does not create a fresh object for every request. Do not put unsynchronized request-specific state in a middleware member. If a callback captures external state by reference, keep that state alive for as long as the web service can invoke the callback.

This example is synchronous. Middleware that starts asynchronous body processing must return `RequestStatus::async` and later complete the context; `MultipartMiddleware` is an example. Do not return `handled` before an asynchronous response is ready.

## Put shared behavior in middleware

Use middleware for request logging, shared header rules, authentication, or parsing uploaded data. Use a controller when the behavior belongs to one feature or route. Tobasa Web's session, authentication, authorization, and multipart middleware are described in [built-in-middlewares.md](built-in-middlewares.md).



