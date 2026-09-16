# Application server endpoints

This is the route list for the application server. It was made from the
`bindHandler()` functions in the controllers.

The paths use the router format. For example, `{user_id:int}` means that the
path must contain a value named `user_id`, and that value must be an integer.
Unless the table says something else, the controller returns an
`http::Result`.

## Authentication

The `Registered auth` or `Auth` column shows the authentication scheme set
when the route is registered:

| Scheme | Meaning |
| --- | --- |
| `NONE` | The route does not ask the controller router for authentication. |
| `COOKIE` | Uses browser/session authentication. |
| `BEARER` | Uses a bearer token, normally a JWT. |
| `BASIC` | Uses HTTP Basic authentication. |

This is the starting rule, not always the final rule. The router also checks
`webapp.webService.routeAuthLists` from `appsettings.json` in
[`Router::setupAuthenticationRule`](../../tobasaweb/src/tobasaweb/router.cpp#L284).
Those settings can remove authentication or require another scheme for a
matching path.

The configuration uses checks such as `starts_with` and `ends_with`. The
router applies those checks to the request path. When a route behaves
unexpectedly, check both the controller registration and the configuration.

## Core pages

These routes are registered by [`CoreController`](../src/core/core_controller.cpp#L33).

| Method | Path | Registered auth | Handler | Purpose |
| --- | --- | --- | --- | --- |
| GET | `/` | `NONE` | `onIndex` | Opens the application home page. |
| GET | `/admin` | `BASIC` | `onAdmin` | Opens the admin page. |
| GET | `/spage` | `NONE` | `onSpage` | Shows an HTTP status page. |
| GET | `/spage/{statusCode:int}` | `NONE` | `onSpage` | Shows a status page for the given status code. |
| GET | `/server_status` | `COOKIE` | `onServerStatus` | Shows the server status page. |
| GET | `/dashboard` | `COOKIE` | `onDashboard` | Shows the user dashboard. |
| GET | `/login` | `NONE` | `onLogin` | Shows the login page. |
| POST | `/login` | `NONE` | `onLogin` | Sends the login form. |
| GET | `/logout` | `NONE` | `onLogout` | Logs out; the `redirect` query option is supported. |
| GET | `/register` | `NONE` | `onRegister` | Shows the registration page. |
| POST | `/register` | `NONE` | `onRegister` | Sends the registration form. |
| GET | `/password` | `NONE` | `onPassword` | Shows the password page. |
| POST | `/password` | `NONE` | `onPassword` | Sends the password form. |
| GET | `/user_profile` | `COOKIE` | `onUserProfile` | Shows the current user's profile. |
| GET | `/user_profile/{profileId}` | `COOKIE` | `onUserProfile` | Shows the profile for the path value `profileId`. |
| GET | `/resource/{resource_type}/{fileName}` | `COOKIE` | `onAppResources` | Returns an application resource. |
| GET | `/keep_alive` | `COOKIE` | `onKeepAlive` | Keeps the current session alive. |
| GET | `/server_event_socket` | `COOKIE` | `onServerEventWebsocket` | Opens the server event WebSocket connection. |
| GET | `/server_event_sse` | `COOKIE` | `onServerEventSse` | Opens the server-sent event connection. |

If no route matches, `CoreController` uses `onIndex` as the router's default
handler.

## Core API

These routes are registered by [`ApiCoreController`](../src/core/api_core_controller.cpp#L38).

| Method | Path | Registered auth | Handler | Request/response notes |
| --- | --- | --- | --- | --- |
| GET | `/api/version` | `NONE` | `onVersion` | Returns server version, build/compiler details, and database version. |
| GET | `/api/server_status` | `BEARER` | `onApiServerStatus` | Returns API server status. |
| POST | `/api/authenticate` | `NONE` | `onAuthenticate` | Starts API authentication. |
| POST | `/api/refresh_auth_token` | `NONE` | `onRefreshAuthToken` | Refreshes a token; expects JSON and reads the refresh-token cookie. |
| GET | `/api/encrypt` | `BEARER` | `onDecryptEncrypt` | Encrypts the `data` query value; configuration can change the effective auth rule. |
| GET | `/api/decrypt` | `BEARER` | `onDecryptEncrypt` | Decrypts the `data` query value; configuration can change the effective auth rule. |
| GET | `/api/read_log/{size:int}/{source}` | `BEARER` | `onReadLog` | Reads `size` log entries from the selected `source`; configuration can change the effective auth rule. |
| GET | `/api/running_configuration` | `BEARER` | `onGetAppConfig` | Returns the running configuration; only the authenticated user named `admin` may use it. |

In the checked-in sample configuration, `/api/encrypt` and `/api/decrypt` are
listed in `noAuthenticationList`, even though their route registration uses
`BEARER`. Check the configuration used by your deployment to know the actual
rule.

## User API

These routes are registered by [`ApiUsersController`](../src/core/api_users_controller.cpp#L29).

| Method | Path | Registered auth | Handler | Purpose |
| --- | --- | --- | --- | --- |
| POST | `/api/users/authenticate` | `NONE` | `onAuthenticate` | Logs a user in. The request body must use `application/json`. |
| POST | `/api/users/register` | `NONE` | `onRegister` | Creates a user from the request body. |
| POST | `/api/users/register_with_image` | `NONE` | `onRegisterWithImage` | Creates a user and uploads an image. |
| GET | `/api/users` | `BEARER` | `onGetAll` | Returns the users. |
| GET | `/api/users/{user_id:int}` | `BEARER` | `onGetById` | Returns one user by numeric ID. |
| GET | `/api/users/exists/{user_name}` | `BEARER` | `onUserExists` | Checks whether a user name is already used. |
| PUT | `/api/users/update_profile` | `BEARER` | `onUpdateProfile` | Updates the authenticated user's profile. |
| POST | `/api/users/update_profile_with_image` | `BEARER` | `onUpdateProfileWithImage` | Updates a profile and uploads an image. |
| DELETE | `/api/users/delete` | `BEARER` | `onDelete` | Deletes a user. |
| GET | `/api/users/{user_id:int}/roles` | `BEARER` | `onRoles` | Returns roles for a numeric user ID. |
| GET | `/api/users/{user_id:int}/profile_image` | `BEARER` | `onProfileImage` | Returns a user's profile image. |
| POST | `/api/users/change_password` | `BEARER` | `onChangePassword` | Changes a password. |
| POST | `/api/users/check_password` | `BEARER` | `onCheckPassword` | Checks a password. |
| POST | `/api/users/reset_password` | `NONE` | `onResetPassword` | Resets a password using the reset request. |
| POST | `/api/users/forgot_password` | `NONE` | `onForgotPassword` | Starts the forgot-password process. |

## Administration

These routes are registered by [`AdminController`](../src/core/core_admin_controller.cpp#L37).
The page routes use cookie authentication. The API and data-changing routes
use bearer authentication.

### User administration

| Method | Path | Auth | Handler | Purpose |
| --- | --- | --- | --- | --- |
| GET | `/admin/users` | `COOKIE` | `onUsers` | Opens the user administration page. |
| POST | `/api/admin/users` | `BEARER` | `onUsersPost` | Creates or updates an administrator user. |
| DELETE | `/api/admin/users` | `BEARER` | `onUsersDel` | Deletes administrator user data. |
| POST | `/api/admin/users/reset_password` | `BEARER` | `onUsersResetPasswordPost` | Resets an administrator user's password. |

### Role administration

| Method | Path | Auth | Handler | Purpose |
| --- | --- | --- | --- | --- |
| GET | `/admin/roles` | `COOKIE` | `onRoles` | Opens the role administration page. |
| POST | `/api/admin/roles` | `BEARER` | `onRolesPost` | Creates or updates roles. |
| DELETE | `/api/admin/roles` | `BEARER` | `onRolesDel` | Deletes roles. |
| GET | `/api/admin/roles/get_non_member` | `BEARER` | `onRolesGetNonMember` | Returns users outside the requested role; reads the `roleid` query value. |
| GET | `/api/admin/roles/get_member` | `BEARER` | `onRolesGetMember` | Returns users inside the requested role; reads the `roleid` query value. |
| POST | `/api/admin/roles/addusers` | `BEARER` | `onRolesAddUserPost` | Adds users to a role. |
| POST | `/api/admin/roles/remove_user` | `BEARER` | `onRolesRemoveUserPost` | Removes a user from a role. |

### Menu and ACL administration

| Method | Path | Auth | Handler | Purpose |
| --- | --- | --- | --- | --- |
| GET | `/admin/menus` | `COOKIE` | `onMenus` | Opens the menu administration page. |
| POST | `/api/admin/menus` | `BEARER` | `onMenusPost` | Creates or updates menus. |
| DELETE | `/api/admin/menus` | `BEARER` | `onMenusDel` | Deletes menus. |
| GET | `/admin/acl` | `COOKIE` | `onAcl` | Opens the ACL administration page. |
| POST | `/api/admin/acl` | `BEARER` | `onAclPost` | Creates or updates ACL entries. |
| DELETE | `/api/admin/acl` | `BEARER` | `onAclDel` | Deletes ACL entries. |

## LIS endpoints

These routes are available only when the application is built with
`TOBASA_USE_LIS_ENGINE` and the LIS module starts successfully. `AppLisModule`
registers the controller, which is implemented by
[`ApiLisController`](../src/lis/api_lis_controller.cpp#L68).

| Method | Path | Auth | Handler | Purpose |
| --- | --- | --- | --- | --- |
| GET | `/lis/server_status` | `COOKIE` | `onLisServerStatus` | Opens the LIS status page. |
| GET | `/api/lis/server_status` | `BEARER` | `onApiServerStatus` | Returns LIS status through the API. |
| POST | `/api/lis/start_engine` | `BEARER` | `onApiStartEngine` | Starts the LIS engine. |
| POST | `/api/lis/stop_engine` | `BEARER` | `onApiStopEngine` | Stops the LIS engine. |
| POST | `/api/lis/send_hl7_message` | `BEARER` | `onApiSendHL7Message` | Sends an HL7 message. |
| POST | `/api/lis/send_lis_message` | `BEARER` | `onApiSendLisMessage` | Sends a LIS message. |
| POST | `/api/lis/parse_and_send_message` | `BEARER` | `onApiTestParseAndSendMessage` | Parses a test message and sends it. |
| GET | `/api/lis/lis2a_result_list/{header_id}` | `BEARER` | `onApiLis2aResultList` | Returns LIS2A results for `header_id`. |
| GET | `/api/lis/hl7_obxlist/{obrid}/{patientid}` | `BEARER` | `onApiHl7ObxList` | Returns HL7 OBX results for an OBR and patient. |
| GET | `/lis/testdev_lis1a` | `COOKIE` | `onLisDeviceTestDevLIS1A` | Opens the LIS1A development result page when the matching instrument is configured. |
| GET | `/lis/testdev_hl7` | `COOKIE` | `onLisDeviceTestDevHL7` | Opens the HL7 development result page when the matching instrument is configured. |

The two development pages accept `startdate`, `enddate`, `offset`, `limit`, and
`filter`. Their controller comments describe the expected values.

## Test endpoints

These routes are registered only when `TOBASA_USE_TESTS_MODULE` is enabled by
[`TestController`](../src/test/test_controller.cpp#L62). They are for local or
development testing and are normally not part of a production build.

| Method | Path | Registered auth | Handler | Purpose |
| --- | --- | --- | --- | --- |
| POST | `/test/crypto` | `NONE` | `onCrypto` | Runs the cryptography test. |
| POST | `/test/datetime` | `NONE` | `onDateTime` | Runs the date/time test. |
| POST | `/test/pgsql` | `NONE` | `onSql` | Tests PostgreSQL access. |
| POST | `/test/adosql` | `NONE` | `onSql` | Tests ADODB access. |
| POST | `/test/sqlite` | `NONE` | `onSql` | Tests SQLite access. |
| POST | `/test/odbc_mssql` | `NONE` | `onSql` | Tests SQL Server through ODBC. |
| POST | `/test/odbc_mysql` | `NONE` | `onSql` | Tests MySQL through ODBC. |
| POST | `/test/mysql` | `NONE` | `onSql` | Tests MySQL access. |
| POST | `/test/upload` | `NONE` | `onUpload` | Tests file upload handling. |

## WebSocket endpoints

These routes are added only by the test WebSocket controller when the tests
module is enabled. The controller is implemented by
[`WebsocketController`](../src/test_ws/websocket_controller.cpp#L43).

They are registered as `GET` routes. The handler checks the upgrade request and
then starts WebSocket processing.

| Method | Path | Auth | Handler | Purpose |
| --- | --- | --- | --- | --- |
| GET | `/test_websocket` | `COOKIE` | `onTestWebSocket` | Opens the WebSocket test page. |
| GET | `/websocket_ep` | `COOKIE` | `onWebSocketA` | Opens WebSocket endpoint A. |
| GET | `/websocket_ep_1` | `COOKIE` | `onWebSocketB` | Opens WebSocket endpoint B. |

## Route caveats

- The auth value in a table is the route default. `routeAuthLists` can change
  the effective authentication rule.
- Put typed values such as `{size:int}`, `{statusCode:int}`, and
  `{user_id:int}` in the path. Do not send them as query parameters.
- The LIS values `{header_id}`, `{obrid}`, and `{patientid}` are also path
  values.
- The available routes depend on the build. LIS and test/WebSocket routes are
  missing when their controllers are not compiled and registered.
- `/api/running_configuration` can expose secrets. Its handler allows only the
  authenticated user named `admin`.
