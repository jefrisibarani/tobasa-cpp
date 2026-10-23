# HTTP Sessions in Tobasa Web

The browser sends a session ID cookie; Tobasa Web uses that ID to find JSON stored on the server. The cookie carries an identifier, not the session data and not proof that the user is authenticated.

`Webapp::useSession()` adds `SessionMiddleware` to the request chain. After it has processed a request, a handler can use the session ID attached to `http::Request`:

```cpp
auto session = web::Session::get(httpCtx->sessionId());
if (!session->loaded())
   return http::statusResultHtml(http::StatusCode::UNAUTHORIZED);

auto cartId = session->getData("cart_id");
session->setData("last_page", httpCtx->request()->path());
```

`Session::get()` returns a `Session` object even when there is no stored
session, so check `loaded()` before treating it as existing session data.
`getData()` returns JSON; `setData()` immediately saves through the selected
session store. The implementation is in [session.cpp](../src/tobasaweb/session.cpp).

## How the middleware restores a login

On each request, `SessionMiddleware` first checks `ignoreHandler` and
`noSessionList`. It then reads the `tbs_session` cookie. If session handling
applies and there is no usable ID, it generates an ID, creates a session in
the selected store, sets `Request::sessionId()`, and adds a cookie to the
response. Cookie IDs are checked for length; that check alone does not verify
that a cookie value is a valid UUID.

For a loaded session, the middleware restores an identity only when `logged_in` is true and `expires` is a positive time in the future. It reloads the user through `AuthDbRepo` and writes the identity to `AuthResult`. A missing file, missing expiry, expired time, or missing user does not authenticate the request. Authentication and authorization are separate middleware steps; see [built-in-middlewares.md](built-in-middlewares.md).

Session work can be skipped by `ignoreHandler`, by a `noSessionList` rule, or for most routes whose effective auth scheme is `NONE`. `/`, `/login`, and the configured login path are exceptions. Skipping session work does not skip authentication or authorization. `SessionMiddlewareOption` and the request logic are defined in [session_middleware.h](../include/tobasaweb/session_middleware.h) and [session_middleware.cpp](../src/tobasaweb/session_middleware.cpp).

The middleware does not log users in or out. After a successful login, the
application writes identity and expiry fields. The app server does this in
`setupSessionAndCookie()` in [app_util.cpp](../../app_server/src/app_util.cpp).
Its `logoutAndClearCookie()` clears the request auth state, removes the
session from the selected store, and expires cookies.

## Storage, concurrency, and expiry

`webapp.webService.sessionStore` selects the backend. It defaults to
`filesystem`, which stores JSON files named `tbs_session_<id>` under
`sessionSavePath` (default `./appdata/session`). If the configured directory
is unavailable, the filesystem store uses its executable-relative default.
The files are plain JSON, so protect the directory and avoid storing secrets
there without additional protection.

The app server also supports the `session_store_sqlite_plugin` backend. The
values `sqlite` and `sqlite3` resolve to that plugin ID. The plugin is built
and copied beside the app server only when `TOBASA_BUILD_SHARED_LIB=ON`; the
app server scans `TOBASA_PLUGIN_DIR` and its executable-relative `plugins`
directory before session access. The plugin stores sessions in
`<webapp.webService.dataDir>/session_store.db`. A configured plugin backend
that cannot be found or cannot create a store causes session initialization to
fail; it does not silently fall back to filesystem storage. Custom backends
implement `ISessionStore` and are provided through `SessionStorePlugin`.
See [session_store_plugin.h](../include/tobasaweb/session_store_plugin.h).

`sessionSavePath` applies to filesystem storage; the SQLite plugin uses
`dataDir` for its database location. See [settings_webapp.h](../include/tobasaweb/settings_webapp.h).

`setData()` saves the whole JSON state. A mutex protects each save, but the read-modify-save sequence is not one transaction. Two overlapping requests can both read old state and the later save can overwrite the earlier request's changes. Do not assume session updates from concurrent requests are merged.

`webService.sessionExpirationMinutes` defaults to 15. The login helper uses
10 minutes when the configured value is zero or negative, despite the setting
comment describing zero as browser-close expiry. The filesystem store has an
explicit `cleanupExpired()` operation, but the app-server startup cleanup
currently walks filesystem session files directly and the session layer does
not call the store cleanup operation. The SQLite plugin reports expired
records on load; it does not automatically delete them through that interface.

## Cookie attributes

The initial cookie from `Session::createDefaultCookie()` sets only
`tbs_session=<id>; Path=/`. After login, the app server adds `Max-Age`,
`Secure`, and `SameSite=Strict`, but not `HttpOnly`, to the session cookie.
The refresh-token cookie does include `HttpOnly`. The generic `Cookie` class
can emit `HttpOnly`, but `SessionMiddlewareOption` has no direct switch for
the built-in session cookie. Review these attributes for your deployment; see
[session.h](../include/tobasaweb/session.h) and
[app_util.cpp](../../app_server/src/app_util.cpp).
