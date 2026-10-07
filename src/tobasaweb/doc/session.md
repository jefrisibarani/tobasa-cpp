# HTTP Sessions in Tobasa Web

The mental model is simple: the browser sends a session ID cookie; Tobasa Web uses that ID to find JSON stored on the server. The cookie carries an identifier, not the session data and not proof that the user is authenticated.

`Webapp::useSession()` adds `SessionMiddleware` to the request chain. After it has processed a request, a handler can use the session ID attached to `http::Request`:

```cpp
auto session = web::Session::get(httpCtx->sessionId());
if (!session->loaded())
   return http::statusResultHtml(http::StatusCode::UNAUTHORIZED);

auto cartId = session->getData("cart_id");
session->setData("last_page", httpCtx->request()->path());
```

`Session::get()` returns a `Session` object even when there is no readable file, so check `loaded()` before treating it as existing session data. `getData()` returns JSON; `setData()` writes the updated JSON file immediately. The implementation is in [session.cpp](../src/tobasaweb/session.cpp).

## How the middleware restores a login

On each request, `SessionMiddleware` first checks `ignoreHandler` and `noSessionList`. It then reads the `tbs_session` cookie. If session handling applies and there is no usable ID, it generates an ID, creates a session file, sets `Request::sessionId()`, and adds a cookie to the response. If the cookie value has 36 characters, the middleware sets the request session ID and attempts to load the file. The current check is length-based; it does not verify the cookie value is a UUID.

For a loaded session, the middleware restores an identity only when `logged_in` is true and `expires` is a positive time in the future. It reloads the user through `AuthDbRepo` and writes the identity to `AuthResult`. A missing file, missing expiry, expired time, or missing user does not authenticate the request. Authentication and authorization are separate middleware steps; see [built-in-middlewares.md](built-in-middlewares.md).

Session work can be skipped by `ignoreHandler`, by a `noSessionList` rule, or for most routes whose effective auth scheme is `NONE`. `/`, `/login`, and the configured login path are exceptions. Skipping session work does not skip authentication or authorization. `SessionMiddlewareOption` and the request logic are defined in [session_middleware.h](../include/tobasaweb/session_middleware.h) and [session_middleware.cpp](../src/tobasaweb/session_middleware.cpp).

The middleware does not log users in or out. After a successful login, the application writes identity and expiry fields. The app server does this in `setupSessionAndCookie()` in [app_util.cpp](../../app_server/src/app_util.cpp). Its `logoutAndClearCookie()` clears the request auth state, deletes the session file, and expires cookies.

## Storage, concurrency, and expiry

Each session is a JSON file named `tbs_session_<id>`. `webService.sessionSavePath` sets the directory; its default is `./appdata/session`. If the setting is empty or directory creation fails, Tobasa Web falls back to `appdata/session` beside the executable. Session files are plain JSON, so protect the directory and avoid storing secrets there without additional protection. See [settings_webapp.h](../include/tobasaweb/settings_webapp.h).

`setData()` saves the whole JSON state. A mutex protects each save, but the read-modify-save sequence is not one transaction. Two overlapping requests can both read old state and the later save can overwrite the earlier request's changes. Do not assume session updates from concurrent requests are merged.

`webService.sessionExpirationMinutes` defaults to 15. `Webapp` removes old files at startup using their modification time; the middleware also checks a logged-in session's stored `expires` time on requests. There is a mismatch for zero: the setting comment says it means browser-close expiry, but the app-server login helper changes any non-positive value to 10 minutes. Do not rely on zero for browser-session behavior without fixing both paths.

## Cookie attributes

The initial cookie from `Session::createDefaultCookie()` sets only `tbs_session=<id>; Path=/`. After login, the app server sends it again with `Max-Age`, `Secure`, and `SameSite=Strict`, but not `HttpOnly`. The generic `Cookie` class can emit `HttpOnly`, but `SessionMiddlewareOption` has no direct switch for the built-in session cookie. Review these attributes for your deployment; see [session.h](../include/tobasaweb/session.h) and [app_util.cpp](../../app_server/src/app_util.cpp).
