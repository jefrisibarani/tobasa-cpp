# Register Routes in Tobasa Web

A route is a method, a path pattern, and a callback. The router keeps the callback and calls it when an incoming request matches. Add routes in your controller's `bindHandler()`; the controller factory calls that method during app setup.

## Register one route

The four route methods are `httpGet()`, `httpPost()`, `httpPut()`, and `httpDelete()`. Each takes the path, handler, optional auth scheme, and optional match rule:

```cpp
void UsersController::bindHandler()
{
   using namespace std::placeholders;

   router()->httpGet(                                    // Route method
      "/api/users/{user_id:int}",                        // Path
      std::bind(&UsersController::onGetById, this, _1),  // Handler
      http::AuthScheme::BEARER,                          // Authentication scheme
      ""                                                 // Optional Match rule
   );
}
```

The handler accepts a `const web::RouteArgument&` and returns an `http::ResultPtr`. The app-server uses this pattern in [api_users_controller.cpp](../../app_server/src/core/api_users_controller.cpp).

## Match the path

A request must match both the registered HTTP method and path. The path is split into slash-separated segments. Fixed segments must match exactly; a placeholder captures one segment:

```text
/api/users/{user_id:int}
```

Write a placeholder as `{name}` or `{name:type}`. The name is the key used by the handler. The type is optional and defaults to `string`. A placeholder must fill a whole segment; `user-{id}` is literal text, not a parameter.

For example, `/users/{user_id:int}/orders/{order_id}` matches `/users/42/orders/AB7`. It captures `42` as `user_id` and `AB7` as `order_id`. Each placeholder captures just one segment; it does not capture additional segments after a slash.

The request and template must have the same number of segments. Each captured value must pass its validator. A failure means this route does not match; the router tries later routes, then calls the default handler or returns 404.

One implementation limit can surprise you: the current parser compares the first path segment literally before it parses placeholders. Start templates with a fixed segment, such as `/users/{user_id:int}`. A leading placeholder such as `/{user_id:int}/users` will not match as expected.

The `{user_id:int}` placeholder requires a digits-only segment. Other supported placeholder types are:

- `{name}` or `{name:string}`: a non-empty string segment; `string` is the default. It rejects `.` and `..`, slashes, and control characters.
- `{name:int}`: digits only.
- `{name:alpha}`: letters only.
- `{name:alnum}`: letters and digits.
- `{name:hex}`: hexadecimal digits.
- `{name:uuid}`: 32 hexadecimal digits in groups (`8-4-4-4-12`) separated by hyphens. The validator checks this shape, not UUID version or variant bits.
- `{name:slug}`: lowercase letters, digits, hyphens, or underscores.

These types validate the text; they do not convert it. For example, `int` accepts digits only, so it rejects negative numbers, and `get("user_id")` still returns `std::optional<std::string>`. Check the optional and convert the string in your handler if you need a numeric value. Named values are usually clearest. `get(position)` uses a zero-based index among captured placeholders.

The router removes an optional trailing slash when registering a route. The placeholder parser and validators are implemented in [path_argument.h](../../tobasahttp/include/tobasahttp/path_argument.h) and [validators.h](../../tobasahttp/include/tobasahttp/validators.h).

## Read values and return a result

The handler receives one `RouteArgument` and must return a non-null `http::ResultPtr`. The router immediately calls methods on the returned pointer; returning an empty pointer is not a supported way to signal “not found.”

Here is a complete example for the `/api/users/{user_id:int}` route above:

```cpp
http::ResultPtr UsersController::onGetById(const web::RouteArgument& args)
{
   auto userId = args.get("user_id");
   if (!userId)
   {
      auto result = std::make_shared<http::Result>("Missing user_id", "text/plain");
      result->statusCode(http::StatusCode::BAD_REQUEST);
      return result;
   }

   return std::make_shared<http::Result>("User id: " + *userId, "text/plain");
}
```

The `int` path type validates digits but `get()` still returns a string. The missing-value check is defensive: a successful normal match with this placeholder has already captured the value.

Use `get("name")` to read a named value as an optional string, `get(position)` to read a captured value by zero-based position, and `httpContext()` to access the current request context. `RouteArgument` is declared in [router_base.h](../include/tobasaweb/router_base.h). A route without placeholders still receives a `RouteArgument`, but it has no path values.

## Set authentication

The third argument is an `http::AuthScheme`. It defaults to `http::AuthScheme::NONE`. The available values are `NONE`, `BASIC`, `BEARER`, `COOKIE`, and `CUSTOM`; see [authentication.h](../../tobasahttp/include/tobasahttp/authentication.h).

For example, a route can declare that it uses Bearer authentication:

```cpp
router()->httpGet(
   "/api/users",
   std::bind(&UsersController::onGetAll, this, std::placeholders::_1),
   http::AuthScheme::BEARER);
```

The router uses this value when it sets up authentication for a matching request. Web service authentication rules in configuration can also affect the effective scheme. `NONE` means this route does not set a scheme; it does not prevent a configuration rule from requiring authentication.

## Set a match rule

The fourth argument is an optional string and defaults to empty. Leave it empty for normal path matching. The only special value is `"starts_with"`:

```cpp
router()->httpGet(
   "/assets",
   std::bind(&AssetsController::onAssetRequest, this, std::placeholders::_1),
   http::AuthScheme::NONE,
   "starts_with");
```

This is a literal prefix check, not a path-segment check: `"/assets"` also matches `"/assets-old"`. It does not capture the remaining path. Use named placeholders to pass values to the handler. The first matching route wins, so register specific routes before broad prefix routes.

The router stores the callback in its route table. In a controller, callbacks commonly bind `this`; the controller factory retains the controller while the web service retains its factories. If a callback captures other objects by reference, those objects must also outlive the route. See [controller.md](controller.md) for the controller lifetime.

## Route selection

The router checks routes in registration order and chooses the first method-and-path match. It removes a trailing slash when registering a route. If no route matches, it calls the default handler or returns 404. See [router.md](router.md) for dispatch details and the default-handler replacement rule. Matching is implemented in [router.cpp](../src/tobasaweb/router.cpp); placeholder parsing is in [path_argument.cpp](../../tobasahttp/src/tobasahttp/path_argument.cpp).
