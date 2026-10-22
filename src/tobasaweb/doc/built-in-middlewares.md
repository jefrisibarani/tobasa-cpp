# Built-in Middleware in Tobasa Web

Each built-in helper adds a request-processing step to the web service. The steps run in registration order, followed by the router. Use only the helpers your app needs:

```cpp
webapp.useMultipart();
webapp.useSession();
webapp.useCsrf();
webapp.useAuthentication();
webapp.useAuthorization();
```

The app-server setup is in [main.cpp](../../app_server/src/main.cpp). Register session before CSRF, authentication, and authorization. Register multipart parsing before CSRF when request forms may be multipart. `useAuthentication()` also inserts a small step that initializes `AuthResult` before the authentication middleware. The helpers and their option-builder callbacks are declared in [webapp.h](../include/tobasaweb/webapp.h).

## Validate browser requests

`useCsrf()` adds `CsrfMiddleware`. It creates a URL-safe random token in a loaded server-side session during safe requests (`GET`, `HEAD`, or `OPTIONS`). Other methods require that token either in the `_csrf` form field or the `X-CSRF-Token` request header. Missing sessions, missing tokens, or mismatched tokens are rejected with HTTP 403. The comparison is constant-time for the submitted token lengths.

The middleware does not add the token to rendered pages. The application must read the `csrf_token` value from the session and render it into each protected form or expose it to its JavaScript client. For example:

```html
<input type="hidden" name="_csrf" value="{{ csrfToken }}">
```

Use `CsrfMiddlewareOption::protectHandler` to select requests that require tokens. When it is unset, all routes are selected. `ignoreHandler` bypasses the middleware for requests that match it. Configure these handlers consistently with `SessionMiddlewareOption::forceSessionHandler`, because a protected browser route needs a session even before the user signs in. Bearer-token APIs should normally be excluded from cookie-based CSRF checks; cookie-authenticated API endpoints need an explicit policy.

The app server configures CSRF for browser cookie-authenticated routes and selected public forms. Its cookie-based `/api/refresh_auth_token` endpoint is not covered by that policy and needs separate CSRF protection if it continues authenticating from a cookie.

## Restore a signed-in user

`useSession()` adds `SessionMiddleware`. It reads the `tbs_session` cookie, loads the server-side session, and restores a valid logged-in user's identity into `AuthResult`. The login itself is application code; see [session.md](session.md) for the session lifecycle and stored data.

Use `SessionMiddlewareOption::ignoreHandler` or `noSessionList` to skip session work for selected requests. A `noSessionList` rule can match a path exactly, by prefix (`starts_with`), or by suffix (`ends_with`). Skipping session work does not skip authentication or authorization. Cookie and storage details are in [session.md](session.md).

## Check credentials and permissions

`AuthenticationMiddleware` checks credentials according to the route's effective auth scheme. The built-in checks handle Basic credentials, Bearer JWTs, and cookie authentication; Basic and Bearer checks use `AuthDbRepo`. In the current implementation, Bearer validation is only run for paths starting with `/api` or `/chat_app_socket`.

Configure authentication with `AuthenticationMiddlewareOptionBuilder`. It can supply an `authHandler`, an ignore callback, a login-path callback, cookie settings, or a custom result builder. An ignored authentication request is marked in the request's auth context, and authorization skips it too. The route's auth scheme affects the check; see [registering_route.md](registering_route.md).

`useAuthorization()` expects an `AuthResult` to be present in the request context. For requests with an auth scheme, it allows superusers and site administrators or checks access to the request path and route template through `AuthDbRepo`. `AuthScheme::NONE` continues without a permission check. Configure ignored requests, cookie/login behavior, or a custom error result with `AuthorizationMiddlewareOptionBuilder`.

If you use authorization without earlier middleware that initializes `AuthResult`, its `std::any_cast` has no value to cast and can throw. Keep the normal session/authentication/authorization order unless your own earlier middleware provides the expected state.

## Parse multipart uploads

`useMultipart()` adds `MultipartMiddleware` to parse multipart request bodies. It streams the body through `MultipartParser`, attaches the fields and uploaded files to the request, then continues the chain. Other requests pass through unchanged. `MultipartMiddlewareOption::temporaryDir` sets the directory used for temporary files.

Check `httpServer.enableMultipartParsing` before relying on this middleware. When it is `true`, the HTTP server parses multipart bodies itself and `Webapp::useMultipart()` returns without adding `MultipartMiddleware`. Set it to `false` to use the middleware parser. Multipart parsing reads the body asynchronously; it returns `RequestStatus::async` and resumes the chain after parsing. The setting is declared in [settings_http_server.h](../include/tobasaweb/settings_http_server.h); parser behavior is in [multipart_middleware.cpp](../src/tobasaweb/multipart_middleware.cpp).
