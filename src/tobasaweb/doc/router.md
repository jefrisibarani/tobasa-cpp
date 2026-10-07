# Route Dispatch in Tobasa Web

The router is the last middleware in the normal web-service chain. It turns a request into a call to one stored route callback. Route registration is covered in [registering_route.md](registering_route.md).

## Request flow

At request entry, `MiddlewareManager` asks the router to set up route and auth information. It then runs the registered middleware in order. If they continue, the router matches the method and path, builds `RouteArgument`, calls the handler, and converts the returned result into a response. See [middleware_manager.cpp](../src/tobasaweb/middleware_manager.cpp), [web_service.cpp](../src/tobasaweb/web_service.cpp), and [router.cpp](../src/tobasaweb/router.cpp).

Routes are checked in registration order. The first entry that matches both method and path is selected. The path values are validated during matching, then read through `RouteArgument`. If the handler returns an empty `http::ResultPtr`, the router dereferences it; every route handler must return a valid result.

## When the router is added

`WebService` creates a default router when it is constructed. During `setupHandlers()`, it initializes middleware factories, initializes controller factories (which register their routes), then initializes the router and adds it to the middleware manager as the final middleware. Supply a custom router factory with `useRouter()` before handler setup if the default router should be replaced. See [web_service.cpp](../src/tobasaweb/web_service.cpp) and [router_factory.h](../include/tobasaweb/router_factory.h).

## Set a fallback

Use `defaultHandler()` when unmatched requests need application-specific handling:

```cpp
router()->defaultHandler(
	std::bind(&CoreController::onIndex, self, std::placeholders::_1));
```

The callback receives the HTTP context through `RouteArgument`, but no path values because there is no matched route template. It must return a non-null result. The app server uses this fallback to serve its index and static resources while keeping unmatched API paths as 404s; see [CoreController::onIndex](../../app_server/src/core/core_controller.cpp).

Without a custom fallback, the router returns its built-in 404 result. Only one fallback is stored: every call to `defaultHandler()` replaces the previous callback. Set it once in a central place.

The selected route's auth scheme is placed in the request auth context during setup. Configuration rules can change the effective scheme; the authentication middleware reads that value. See [built-in-middlewares.md](built-in-middlewares.md).


