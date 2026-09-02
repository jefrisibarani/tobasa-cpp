# `appsettings.json` configuration reference

This document describes the configuration objects used by the application
server. It is based on the two checked-in files:

- [`configuration/appsettings.json`](../configuration/appsettings.json): the
	local/runtime profile.
- [`configuration_embed/appsettings.json`](../configuration_embed/appsettings.json):
	the portable configuration compiled into the executable.

Both files have the same object model. Their values are different profiles,
not two files that are merged together. The runtime file is loaded first; the
embedded file is used only when the runtime file cannot be opened.

## Processing rules

The application loads the main file into [`tbs::Config`](../../tobasa/include/tobasa/config.h#L16) during startup, using [`Config::load`](../../tobasa/src/config.cpp#L26). The
loader accepts comments in JSON and performs placeholder substitution in
string values while parsing.

### Configuration variables

`configVariables` is a map of string-to-string substitutions. A placeholder
is written as `${NAME}` and must be present as a key in this object. The
placeholder can appear more than once in a string, and a string can contain
multiple different placeholders.

```json
"configVariables": {
	"${WS_DATADIR}": "./appdata",
	"${WS_LOGDIR}": "./appdata/log",
	"${DBSUFFIX}": ""
}
```

These are not operating-system environment variables. They are values defined
by the JSON file. An undefined placeholder causes configuration loading to
fail. Do not include a trailing slash in directory variable values when the
value is later followed by `/`.

### Values after startup

[`Webapp::loadConfig`](../../tobasaweb/src/tobasaweb/webapp.cpp#L187) copies `securitySalt` into
`webapp.dbConnection.securitySalt`. It also resolves the HTTP temporary
directory and TLS file paths relative to the executable and creates the
temporary directory when needed. The path normalization is applied to the
deserialized `Webapp` option; the salt assignment is also written into the
JSON held by `Config`.

The JSON object can be read in C++ with [`Config::getOption`](../../tobasa/include/tobasa/config.h#L76) and [`Config::getNestedOption`](../../tobasa/include/tobasa/config.h#L159):

```cpp
auto webapp = tbs::Config::getOption<tbs::web::conf::Webapp>("webapp");
auto logging = tbs::Config::getOption<tbs::log::conf::Logging>("logging");
auto port = tbs::Config::getNestedOption<int>("webapp.httpServer.port");
```

## Top-level objects

| Property | JSON type | C++ type | Description |
| --- | --- | --- | --- |
| `configVariables` | object | `map<string, string>` internally | String substitutions used during parsing. |
| `securitySalt` | string | `std::string` | Application salt used by authentication, encryption helpers, and database password operations. |
| `webapp` | object | [`tbs::web::conf::Webapp`](../../tobasaweb/include/tobasaweb/settings_webapp.h#L86) | Database, HTTP server, and web-service settings. |
| `logging` | object | [`tbs::log::conf::Logging`](../../tobasaweb/include/tobasaweb/settings_log.h#L29) | stdout and file logger settings. |

Secrets in this document's sample files are configuration values, not
placeholders for machine environment variables. Replace them for a real
deployment and protect the files.

## `webapp`

| Property | JSON type | Source default | Description |
| --- | --- | --- | --- |
| `environment` | string | `"development"` for the option type | Selects the database profile used by the web application.|
| `dbConnection` | object | none | Database profiles and SQL logging settings. |
| `httpServer` | object | `Server` defaults | HTTP/HTTPS server settings. |
| `webService` | object | `WebService` defaults | Sessions, routes, JWT, and application directories. |
| `dbConnectionPoolSize` | integer | none in the struct | Database connection pool size. |

`environment` is a member of `dbConnection`, not of the outer `Webapp`
object. The outer `webapp.environment` value is present in both JSON files,
but the source `Webapp` struct does not serialize that member. The database
selection value consumed by `DbServiceFactory` is
`webapp.dbConnection.environment`.

### `webapp.dbConnection`

This object is [`tbs::sql::conf::ConnectorOption`](../../tobasasql/include/tobasasql/settings.h#L43). It contains two named
database profiles, allowing the active profile to be selected by
`environment`.

| Property | JSON type | Description |
| --- | --- | --- |
| `production` | object | Database settings for the production profile. |
| `development` | object | Database settings for the development profile. |
| `environment` | string | Profile name, normally `"production"` or `"development"`. |
| `logInternalSqlQuery` | boolean | Enables internal SQL query logging. |
| `logSqlQuery` | boolean | Enables SQL query logging. |
| `securitySalt` | string | Optional per-connection salt. Startup overwrites it with the top-level `securitySalt`. |

Each profile is a [`tbs::sql::conf::Database`](../../tobasasql/include/tobasasql/settings.h#L33) object:

| Property | JSON type | Accepted values/meaning |
| --- | --- | --- |
| `dbDriver` | string | `SQLITE`, `MYSQL`, `PGSQL`, `ODBC`, or `ADODB`. |
| `connectionString` | string | Driver-specific connection string without the password. Placeholders are expanded before use; the framework appends the decrypted `password` value. |
| `password` | string | Database password or encrypted password, depending on the database utility path. |


#### `connectionString` syntax by driver

The framework starts with `connectionString`, decrypts the profile's
`password` using `dbConnection.securitySalt`, and appends the password in a
driver-specific form. Keep credentials out of `connectionString` and put them
in the sibling `password` property. An empty `password` is still appended.

##### SQLite

SQLite uses semicolon-delimited parameters parsed by Tobasa:

```json
"dbDriver": "SQLITE",
"connectionString": "Database=${WS_DATADIR}/tobasa_base${DBSUFFIX}.db3;OpenCreate=True;OpenMemory=False;",
"password": ""
```

Supported parameters recognized by the SQLite connection implementation are:

| Parameter | Value | Meaning |
| --- | --- | --- |
| `Database` | path or `:memory:` | Database file. `OpenMemory=True` forces `:memory:`. |
| `OpenReadOnly` | `True` or `False` | Open read-only. |
| `OpenReadWrite` | `True` or `False` | Open read/write. |
| `OpenCreate` | `True` or `False` | Create the database when it does not exist. |
| `OpenMemory` | `True` or `False` | Use an in-memory SQLite database. |
| `Password` | appended by framework | Passed to the SQLite encryption/key step. Do not add it manually. |

The implementation recognizes parameter names by prefix and extracts the
value immediately after the name, so use the shown `Name=Value;` form without
spaces around `=`. The runtime `password` is appended as
`Password=<decrypted-password>;`.

##### PostgreSQL

PostgreSQL uses libpq keyword/value syntax, with fields separated by spaces:

```json
"dbDriver": "PGSQL",
"connectionString": "dbname=tobasa_base${DBSUFFIX} user=tbs_user hostaddr=10.0.0.2 port=5462",
"password": "27CA998DA4C4D345BC0C86F62B7C81BA"
```

The framework appends the password as ` password=<decrypted-password>` before
calling `PQconnectdb`. The connection string therefore follows libpq's
keyword/value rules, including libpq quoting/escaping when a value contains
spaces or special characters. The runtime sample uses `dbname`, `user`,
`hostaddr`, and `port`; other libpq connection keywords may be supplied in
the same string.

##### ODBC

ODBC uses a semicolon-delimited ODBC connection string:

```json
"dbDriver": "ODBC",
"connectionString": "Driver={ODBC Driver 17 for SQL Server};Server=10.0.0.2;Database=tobasa_base${DBSUFFIX};UID=tbs_user;APP=ws_tcxx;TrustServerCertificate=Yes;",
"password": "27CA998DA4C4D345BC0C86F62B7C81BA"
```

The framework appends `Pwd=<decrypted-password>;` and passes the result to
`SQLDriverConnect` with `SQL_DRIVER_NOPROMPT`. Use the driver name installed
on the machine. ODBC attribute names and supported values are supplied by the
selected ODBC driver; for SQL Server, `Driver`, `Server`, `Database`, `UID`,
`APP`, and `TrustServerCertificate` are the attributes shown by the runtime
example.

##### ADODB

ADODB uses an OLE DB provider connection string:

```json
"dbDriver": "ADODB",
"connectionString": "Provider=SQLNCLI11;Server=10.0.0.2;Database=tobasa_base${DBSUFFIX};Uid=tbs_user;DataTypeCompatibility=80;APP=ws_tcxx;",
"password": "27CA998DA4C4D345BC0C86F62B7C81BA"
```

On supported MSVC builds, the framework appends `Pwd=<decrypted-password>;`
to the string and passes it to `ADODB::Connection::Open`. `Provider` selects
the installed OLE DB provider; the remaining attributes are provider-specific.
The runtime example uses SQL Native Client (`SQLNCLI11`). ADODB support is
conditional in the source and is not available on non-MSVC builds.

##### MySQL/MariaDB

MySQL uses semicolon-delimited parameters parsed by Tobasa:

```json
"dbDriver": "MYSQL",
"connectionString": "Database=tobasa_base${DBSUFFIX};User=tbs_user;Server=10.0.0.2;Port=3306;",
"password": ""
```

The framework appends `Password=<decrypted-password>;`. The connection
implementation reads these parameter names:

| Parameter | Meaning |
| --- | --- |
| `Database` | Database/schema name. |
| `User` | User name. |
| `Server` | Host name or address. |
| `Port` | Numeric TCP port. |
| `Password` | Appended by the framework. |

Use the exact `Name=Value;` spelling shown because the Tobasa parser matches
these names by prefix. The resulting values are passed to
`mysql_real_connect`.

##### Password handling summary

The final connection string is constructed as follows:

| Driver | Appended form |
| --- | --- |
| `PGSQL` | ` password=<decrypted-password>` |
| `SQLITE` | `Password=<decrypted-password>;` |
| `ODBC` | `Pwd=<decrypted-password>;` |
| `ADODB` | `Pwd=<decrypted-password>;` |
| `MYSQL` | `Password=<decrypted-password>;` |

The `password` field is passed through `crypt::passwordDecrypt` with the
effective database security salt before appending. If the value is not in the
expected encrypted format, the decryption behavior belongs to the crypto
helper; do not assume that a plaintext password is automatically encrypted.

### `webapp.httpServer`

This object maps to [`tbs::http::conf::Server`](../../tobasaweb/include/tobasaweb/settings_http_server.h#L67).

| Property | JSON type | Default | Description |
| --- | --- | --- | --- |
| `runHttpsOnly` | boolean | `false` | Run HTTPS only. |
| `http2Enabled` | boolean | `false` when HTTP/2 is compiled in | Enable HTTP/2. This field is deserialized only when `TOBASA_HTTP_USE_HTTP2` is defined. |
| `address` | string | `127.0.0.1` | Bind address. |
| `port` | integer | `8084` | HTTP port. |
| `portHttps` | integer | `8085` | HTTPS port. |
| `timeoutRead` | integer | `60` seconds | Read timeout. |
| `timeoutWrite` | integer | `60` seconds | Write timeout. |
| `timeoutProcessing` | integer | `120` seconds | Request-processing timeout. |
| `readBufferSize` | integer | `65536` bytes | Read buffer size. |
| `sendBufferSize` | integer | `65536` bytes | Send buffer size. |
| `maxHeaderSize` | integer | `65536` bytes | Maximum HTTP header size. |
| `docRoot` | string | `./wwwroot` | Web document root; no trailing slash is expected. |
| `temporaryDir` | string | empty, then platform temporary directory | Multipart-processing temporary directory. Resolved relative to the executable. |
| `tls` | object | `Tls` defaults | Server certificate and key settings. |
| `compression` | object | `Compression` defaults | Response compression settings. |
| `ioPoolSize` | integer | `4` | I/O context thread-pool size; `0` disables that pool. |
| `workerPoolSize` | integer | `4` | HTTP request worker-pool size; `0` disables that pool. |
| `logVerbose` | boolean | `false` | Verbose HTTP logging. |
| `logVerboseHttp2` | boolean | `false` when HTTP/2 is compiled in | Verbose HTTP/2 logging. |
| `useRateLimiter` | boolean | `false` | Enable the rate limiter. |
| `rateLimiterMaxRequests` | integer | `10` | Requests allowed in one rate-limit window. |
| `rateLimiterWindowDuration` | integer | `1000` ms | Rate-limit window duration. |
| `rateLimiterBlockDuration` | integer | `30000` ms | Block duration after rate-limit violations. |
| `rateLimiterMaxViolations` | integer | `3` | Violations before blocking. |
| `maxRequestsPerConnection` | integer | `100` | Maximum requests per connection; `0` means unlimited in the runtime profile's comment. |
| `enableMultipartParsing` | boolean | `true` | Enable multipart request parsing. |

#### `webapp.httpServer.tls`

This object maps to [`tbs::http::conf::Tls`](../../tobasaweb/include/tobasaweb/settings_http_server.h#L31).

| Property | JSON type | Default | Description |
| --- | --- | --- | --- |
| `certificateChainFile` | string | `./localhost.crt` | Server certificate chain path. |
| `privateKeyFile` | string | `./localhost.key` | Server private key path. |
| `password` | string | empty | Private-key password. |
| `tmpDhFile` | string | `./dh2048.pem` | Temporary Diffie-Hellman parameter file. |
| `hostCertificates` | array | empty | Host-specific certificate entries. |

Each `hostCertificates` entry has:

| Property | JSON type | Description |
| --- | --- | --- |
| `hostname` | string | Hostname matched by the certificate. |
| `certificateChainFile` | string | Certificate chain path. |
| `privateKeyFile` | string | Private key path. |
| `password` | string | Private-key password. |

TLS paths are resolved relative to the executable during `loadConfig`.

#### `webapp.httpServer.compression`

| Property | JSON type | Default | Description |
| --- | --- | --- | --- |
| `enable` | boolean | `true` | Enable compression. |
| `minimalLength` | integer | `1024` bytes | Minimum response length for compression. |
| `encoding` | string | `gzip` | Compression encoding. |
| `mimetypes` | string | selected text/JSON types | Space-separated MIME types eligible for compression. |

### `webapp.webService`

This object maps to `tbs::web::conf::WebService`.

| Property | JSON type | Default | Description |
| --- | --- | --- | --- |
| `routeAuthLists` | object | none | Authentication rules for routes. |
| `sessionExpirationMinutes` | integer | `15` | Session lifetime. `0` expires when the browser closes. |
| `sessionSavePath` | string | `./appdata/session` | Session file directory; no trailing slash. |
| `acceptedClientAppId` | string | `TBSRESTC_DEV,TBSRESTC_TOBASA` | Comma-separated accepted client application IDs. |
| `authJwtIssuer` | string | `TBS_WEBSVC` | JWT issuer. |
| `authJwtSecret` | string | built-in sample secret | JWT access-token secret. |
| `authJwtSecretRefresh` | string | built-in sample secret | JWT refresh-token secret. |
| `authJwtExpireTimeSpanMinutes` | integer | `15` | Access-token lifetime. |
| `authJwtRefreshExpireTimeSpanMinutes` | integer | `1440` | Refresh-token lifetime. |
| `useInMemoryResources` | boolean | `true` | Use compiled templates and static resources when available. The executable forces the runtime flag to `false` if it was not built with in-memory resources. |
| `templateDir` | string | `./views` | Template directory. |
| `uploadDir` | string | `./appdata/upload` | Upload directory; no trailing slash. |
| `dataDir` | string | `./appdata` | Application data directory; no trailing slash. |
| `homePage` | string | `/dashboard` | Home-page route. |
| `loginPage` | string | `/login` | Login route. |
| `logoutPage` | string | `/logout` | Logout route. |
| `noSessionList` | array | empty | Routes or patterns that do not use sessions. |

#### `routeAuthLists`

| Property | JSON type | Description |
| --- | --- | --- |
| `noAuthenticationList` | array | Routes that do not require authentication. |
| `needAuthenticationList` | array | Routes that require authentication. |

Each route entry contains `path`, `check`, and `authScheme` strings. The
checked-in files use `starts_with` checks and the `bearer` authentication
scheme. The matching behavior is implemented by the web router, so keep the
`check` value consistent with the router's supported checks.

#### `noSessionList`

Each entry contains:

| Property | JSON type | Description |
| --- | --- | --- |
| `path` | string | Path or path fragment to match. |
| `check` | string | Matching operation, for example `ends_with`. |

## `logging`

The `logging` object maps to [`tbs::log::conf::Logging`](../../tobasaweb/include/tobasaweb/settings_log.h#L29).

| Property | JSON type | Description |
| --- | --- | --- |
| `multiSinkLevel` | string | Overall multi-sink level. |
| `stdoutColor` | object | Colored stdout sink. |
| `fileSink` | object | Main file sink. |
| `fileSinkD` | object | Optional detailed/debug file sink. |

Log level strings are `trace`, `debug`, `info`, `warn`, `error`, `critical`,
`off`, or `n_level` as defined by the `spdlog` enum conversion.

Each sink object has:

| Property | JSON type | Description |
| --- | --- | --- |
| `level` | string | Minimum log level for the sink. |
| `pattern` | string | `spdlog` output pattern. |
| `filePath` | string | File path; used by file sinks only. Placeholders are expanded. |
| `truncate` | boolean | Truncate the file when opened. Used by file sinks only. |
| `enable` | boolean | Enable the file sink. Used by file sinks only. |

`stdoutColor` uses only `level` and `pattern`. `fileSink` and `fileSinkD` use
all five properties. The checked-in configuration enables `fileSink` and
disables `fileSinkD`.

## Separate header-rule configuration

`configuration/appsettings_header_rules.json` and its embedded counterpart
are loaded separately after the main file. They are stored in the effective
configuration under `httpResponseHeaderRule`, using
`HttpResponseHeaderRule`.

### `httpResponseHeaderRule`

| Property | JSON type | Description |
| --- | --- | --- |
| `cacheControl` | array | Cache-control rules selected by host and request path. |
| `headerRule` | array | Response-header and CORS rules. |

Each `cacheControl` item contains `host`, `requestPath`, and `rules`. Each
`rules` entry contains a regular-expression `pattern` and the resulting
`header` value.

Each `headerRule` item contains:

| Property | JSON type | Description |
| --- | --- | --- |
| `type` | string | `none` for ordinary response headers or `cors` for CORS headers. |
| `hostOrigin` | array of strings | Hosts, or origins when `type` is `cors`. |
| `requestPath` | array of strings | Paths to which the rule applies. |
| `headers` | array | Header objects containing string `key` and `value`. |


