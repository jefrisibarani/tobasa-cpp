# Application server endpoints

This is the endpoint inventory for the application server assembled from the
`bindHandler()` implementations in the controllers. Paths use the router's
syntax: `{name:type}` is a typed path parameter. Unless stated otherwise,
the endpoint returns the `http::Result` produced by its controller handler.

## Authentication

The authentication value in the tables is the scheme declared when the route
is registered:

| Scheme | Meaning |
| --- | --- |
| `NONE` | No controller-level authentication requirement. |
| `COOKIE` | Browser/session authentication. |
| `BEARER` | Bearer-token authentication. |
| `BASIC` | HTTP Basic authentication. |

The [`Router::setupAuthenticationRule`](../../tobasaweb/src/tobasaweb/router.cpp#L284) also applies `webapp.webService.routeAuthLists` from
`appsettings.json`. A configured route rule can disable authentication or
require an authentication scheme, so the registration value is the default
before configuration rules are applied. The `check` values used by the
configuration (`starts_with`, `ends_with`, and so on) are evaluated by the
router against the request path.

## Core pages

Registered by [`CoreController`](../src/core/core_controller.cpp#L33).

| Method | Path | Registered auth | Handler | Purpose |
| --- | --- | --- | --- | --- |
| GET | `/` | `NONE` | `onIndex` | Application home page. |
| GET | `/admin` | `BASIC` | `onAdmin` | Admin page. |
| GET | `/spage` | `NONE` | `onSpage` | HTTP status page. |
| GET | `/spage/{statusCode:int}` | `NONE` | `onSpage` | HTTP status page for the supplied status code. |
| GET | `/server_status` | `COOKIE` | `onServerStatus` | Server-status page. |
| GET | `/dashboard` | `COOKIE` | `onDashboard` | Dashboard page. |
| GET | `/login` | `NONE` | `onLogin` | Login page. |
| POST | `/login` | `NONE` | `onLogin` | Login form submission. |
| GET | `/logout` | `NONE` | `onLogout` | Logout; the handler supports the `redirect` query option. |
| GET | `/register` | `NONE` | `onRegister` | Registration page. |
| POST | `/register` | `NONE` | `onRegister` | Registration form submission. |
| GET | `/password` | `NONE` | `onPassword` | Password page. |
| POST | `/password` | `NONE` | `onPassword` | Password form submission. |
| GET | `/user_profile` | `COOKIE` | `onUserProfile` | Current user's profile page. |
| GET | `/user_profile/{profileId}` | `COOKIE` | `onUserProfile` | Profile page for the path profile ID. |
| GET | `/resource/{resource_type}/{fileName}` | `COOKIE` | `onAppResources` | Application resource retrieval. |
| GET | `/keep_alive` | `COOKIE` | `onKeepAlive` | Keeps the authenticated session alive. |

If no specific route matches, `CoreController` installs `onIndex` as the
router default handler.

## Core API

Registered by [`ApiCoreController`](../src/core/api_core_controller.cpp#L38) under `/api`.

| Method | Path | Registered auth | Handler | Request/response notes |
| --- | --- | --- | --- | --- |
| GET | `/api/version` | `NONE` | `onVersion` | Returns server version, build/compiler information, and database version. |
| GET | `/api/server_status` | `BEARER` | `onApiServerStatus` | Returns API server status information. |
| POST | `/api/authenticate` | `NONE` | `onAuthenticate` | API authentication request. |
| POST | `/api/refresh_auth_token` | `NONE` | `onRefreshAuthToken` | Refresh-token request; expects JSON content and reads the refresh-token cookie. |
| GET | `/api/encrypt` | `NONE` | `onDecryptEncrypt` | Encrypts the `data` query value; configuration may require bearer auth. |
| GET | `/api/decrypt` | `NONE` | `onDecryptEncrypt` | Decrypts the `data` query value; configuration may require bearer auth. |
| GET | `/api/read_log/{size:int}/{source}` | `NONE` | `onReadLog` | Reads the requested number of log entries from the selected source; configuration may add authentication. |
| GET | `/api/running_configuration` | `BEARER` | `onGetAppConfig` | Returns `Config::get().getConfiguration()`; the authenticated username must also be `admin`. |

The checked-in route configuration explicitly places `/api/encrypt` and
`/api/decrypt` in `noAuthenticationList` with the `bearer` scheme value in
the sample, so deployments should verify their own effective route rules.

## User API

Registered by [`ApiUsersController`](../src/core/api_users_controller.cpp#L29) under `/api/users`.

| Method | Path | Registered auth | Handler | Purpose |
| --- | --- | --- | --- | --- |
| POST | `/api/users/authenticate` | `NONE` | `onAuthenticate` | Authenticates a user. The handler requires an `application/json` request body. |
| POST | `/api/users/register` | `NONE` | `onRegister` | Registers a user from the request body. |
| POST | `/api/users/register_with_image` | `NONE` | `onRegisterWithImage` | Registers a user with an uploaded image. |
| GET | `/api/users` | `BEARER` | `onGetAll` | Lists users. |
| GET | `/api/users/{user_id:int}` | `BEARER` | `onGetById` | Gets a user by numeric ID. |
| GET | `/api/users/exists/{user_name}` | `BEARER` | `onUserExists` | Checks whether a user name exists. |
| PUT | `/api/users/update_profile` | `BEARER` | `onUpdateProfile` | Updates the authenticated user's profile. |
| POST | `/api/users/update_profile_with_image` | `BEARER` | `onUpdateProfileWithImage` | Updates a profile and image. |
| DELETE | `/api/users/delete` | `BEARER` | `onDelete` | Deletes a user. |
| GET | `/api/users/{user_id:int}/roles` | `BEARER` | `onRoles` | Gets roles for a numeric user ID. |
| GET | `/api/users/{user_id:int}/profile_image` | `BEARER` | `onProfileImage` | Gets a user's profile image. |
| POST | `/api/users/change_password` | `BEARER` | `onChangePassword` | Changes a password. |
| POST | `/api/users/check_password` | `BEARER` | `onCheckPassword` | Checks a password. |
| POST | `/api/users/reset_password` | `NONE` | `onResetPassword` | Resets a password using the reset request. |
| POST | `/api/users/forgot_password` | `NONE` | `onForgotPassword` | Starts the forgot-password flow. |

## Administration

Registered by [`AdminController`](../src/core/core_admin_controller.cpp#L37). The page routes use cookie authentication;
the data-changing and data API routes use bearer authentication.

### User administration

| Method | Path | Auth | Handler | Purpose |
| --- | --- | --- | --- | --- |
| GET | `/admin/users` | `COOKIE` | `onUsers` | Renders the user administration page. |
| POST | `/api/admin/users` | `BEARER` | `onUsersPost` | Creates or updates administrative user data. |
| DELETE | `/api/admin/users` | `BEARER` | `onUsersDel` | Deletes administrative user data. |
| POST | `/api/admin/users/reset_password` | `BEARER` | `onUsersResetPasswordPost` | Resets an administrative user's password. |

### Role administration

| Method | Path | Auth | Handler | Purpose |
| --- | --- | --- | --- | --- |
| GET | `/admin/roles` | `COOKIE` | `onRoles` | Renders the role administration page. |
| POST | `/api/admin/roles` | `BEARER` | `onRolesPost` | Creates or updates roles. |
| DELETE | `/api/admin/roles` | `BEARER` | `onRolesDel` | Deletes roles. |
| GET | `/api/admin/roles/get_non_member` | `BEARER` | `onRolesGetNonMember` | Gets users not in the requested role; handler reads the `roleid` query parameter. |
| GET | `/api/admin/roles/get_member` | `BEARER` | `onRolesGetMember` | Gets users in the requested role; handler reads the `roleid` query parameter. |
| POST | `/api/admin/roles/addusers` | `BEARER` | `onRolesAddUserPost` | Adds users to a role. |
| POST | `/api/admin/roles/remove_user` | `BEARER` | `onRolesRemoveUserPost` | Removes a user from a role. |

### Menu and ACL administration

| Method | Path | Auth | Handler | Purpose |
| --- | --- | --- | --- | --- |
| GET | `/admin/menus` | `COOKIE` | `onMenus` | Renders the menu administration page. |
| POST | `/api/admin/menus` | `BEARER` | `onMenusPost` | Creates or updates menus. |
| DELETE | `/api/admin/menus` | `BEARER` | `onMenusDel` | Deletes menus. |
| GET | `/admin/acl` | `COOKIE` | `onAcl` | Renders the ACL administration page. |
| POST | `/api/admin/acl` | `BEARER` | `onAclPost` | Creates or updates ACL entries. |
| DELETE | `/api/admin/acl` | `BEARER` | `onAclDel` | Deletes ACL entries. |

## LIS endpoints

These routes are registered only when the application is built with
`TOBASA_USE_LIS_ENGINE` and the LIS module initializes successfully. The
controller is registered by `AppLisModule` and implemented by
[`ApiLisController`](../src/lis/api_lis_controller.cpp#L68).

| Method | Path | Auth | Handler | Purpose |
| --- | --- | --- | --- | --- |
| GET | `/lis/server_status` | `COOKIE` | `onLisServerStatus` | LIS status page. |
| GET | `/api/lis/server_status` | `BEARER` | `onApiServerStatus` | LIS API status. |
| POST | `/api/lis/start_engine` | `BEARER` | `onApiStartEngine` | Starts the LIS engine. |
| POST | `/api/lis/stop_engine` | `BEARER` | `onApiStopEngine` | Stops the LIS engine. |
| POST | `/api/lis/send_hl7_message` | `BEARER` | `onApiSendHL7Message` | Sends an HL7 message. |
| POST | `/api/lis/send_lis_message` | `BEARER` | `onApiSendLisMessage` | Sends a LIS message. |
| POST | `/api/lis/parse_and_send_message` | `BEARER` | `onApiTestParseAndSendMessage` | Parses and sends a test message. |
| GET | `/api/lis/lis2a_result_list/{header_id}` | `BEARER` | `onApiLis2aResultList` | Gets LIS2A results for a header ID. |
| GET | `/api/lis/hl7_obxlist/{obrid}/{patientid}` | `BEARER` | `onApiHl7ObxList` | Gets HL7 OBX results for an OBR and patient. |
| GET | `/lis/testdev_lis1a` | `COOKIE` | `onLisDeviceTestDevLIS1A` | LIS1A development result view; bound only for the matching configured development instrument. |
| GET | `/lis/testdev_hl7` | `COOKIE` | `onLisDeviceTestDevHL7` | HL7 development result view; bound only for the matching configured development instrument. |

The two development views accept the query parameters documented in their
controller comments: `startdate`, `enddate`, `offset`, `limit`, and `filter`.

## Test endpoints

These routes are registered only when `TOBASA_USE_TESTS_MODULE` is enabled by
[`TestController`](../src/test/test_controller.cpp#L62).
They are development/test surfaces and are not part of a production build's
normal endpoint set.

| Method | Path | Registered auth | Handler | Purpose |
| --- | --- | --- | --- | --- |
| POST | `/test/crypto` | `NONE` | `onCrypto` | Cryptography test. |
| POST | `/test/datetime` | `NONE` | `onDateTime` | Date/time test. |
| POST | `/test/pgsql` | `NONE` | `onSql` | PostgreSQL SQL test. |
| POST | `/test/adosql` | `NONE` | `onSql` | ADODB SQL test. |
| POST | `/test/sqlite` | `NONE` | `onSql` | SQLite SQL test. |
| POST | `/test/odbc_mssql` | `NONE` | `onSql` | SQL Server over ODBC test. |
| POST | `/test/odbc_mysql` | `NONE` | `onSql` | MySQL over ODBC test. |
| POST | `/test/mysql` | `NONE` | `onSql` | MySQL SQL test. |
| POST | `/test/upload` | `NONE` | `onUpload` | Upload test. |

## WebSocket endpoints

These routes are registered only with the test WebSocket controller, which is
conditionally added with the tests module. The controller is implemented by
[`WebsocketController`](../src/test_ws/websocket_controller.cpp#L43). They are
GET route registrations that upgrade to WebSocket handling in the handler.

| Method | Path | Auth | Handler | Purpose |
| --- | --- | --- | --- | --- |
| GET | `/test_websocket` | `COOKIE` | `onTestWebSocket` | WebSocket test endpoint. |
| GET | `/websocket_ep` | `COOKIE` | `onWebSocketA` | WebSocket endpoint A. |
| GET | `/websocket_ep_1` | `COOKIE` | `onWebSocketB` | WebSocket endpoint B. |

## Route caveats

- The route declaration is not the entire authentication decision; configured
	`routeAuthLists` are applied by `Router::setupRoute`.
- Path placeholders are matched by the router. `{size:int}`, `{statusCode:int}`,
	`{user_id:int}`, and the LIS `{header_id}`/`{obrid}`/`{patientid}` placeholders
	should be supplied in the path, not as query parameters.
- The endpoint list is build-dependent. LIS and test/WebSocket routes are
	absent unless their corresponding controllers are compiled and registered.
- The running-configuration endpoint exposes secrets and is restricted in its
	handler to the authenticated user named `admin`.
