# `appsettings.json` configuration reference

This document explains the settings used by the application server. It keeps
the same layout as the configuration file, so you can find a setting quickly
and know what it changes.

The server uses these two files:

- [`configuration/appsettings.json`](../configuration/appsettings.json): the
  normal file used at runtime.
- [`configuration_embed/appsettings.json`](../configuration_embed/appsettings.json):
  a copy built into the executable.

These files are not merged. The server tries the runtime file first. It uses
the embedded file only when it cannot open the runtime file.

## Processing rules

At startup, the application loads the main file into
[`tbs::Config`](../../tobasa/include/tobasa/config.h#L16) with
[`Config::load`](../../tobasa/src/config.cpp#L26). The loader accepts comments
in JSON and replaces the variables defined in `configVariables`.

### Configuration variables

`configVariables` lets you reuse text in several settings. Define a value and
write it as `${NAME}` where you need it:

```json
"configVariables": {
   "${WS_DATADIR}": "./appdata",
   "${WS_LOGDIR}": "./appdata/log",
   "${DBSUFFIX}": ""
}
```

These are variables from the JSON file, not operating-system environment
variables. Every placeholder must have a matching key. An unknown placeholder
makes configuration loading fail.

A variable can appear more than once, and one string can contain several
different variables. Do not add a trailing slash to a directory variable when
the setting adds another `/` after it.

### Values after startup

[`Webapp::loadConfig`](../../tobasaweb/src/tobasaweb/webapp.cpp#L187) does a
little extra work after loading the JSON:

- it copies the top-level `securitySalt` into
  `webapp.dbConnection.securitySalt`;
- it resolves relative temporary-directory and TLS paths from the executable
  directory;
- it creates the temporary directory if it does not exist.

You can read the loaded values in C++ with
[`Config::getOption`](../../tobasa/include/tobasa/config.h#L76) and
[`Config::getNestedOption`](../../tobasa/include/tobasa/config.h#L159):

```cpp
auto webapp = tbs::Config::getOption<tbs::web::conf::Webapp>("webapp");
auto logging = tbs::Config::getOption<tbs::log::conf::Logging>("logging");
auto port = tbs::Config::getNestedOption<int>("webapp.httpServer.port");
```

## Top-level objects

| Property | JSON type | C++ type | Description |
| --- | --- | --- | --- |
| `configVariables` | object | `map<string, string>` internally | Text replacements used while the file is read. |
| `securitySalt` | string | `std::string` | Salt used by authentication, encryption helpers, and database password handling. |
| `webapp` | object | [`tbs::web::conf::Webapp`](../../tobasaweb/include/tobasaweb/settings_webapp.h#L86) | Database, HTTP server, and web-service settings. |
| `logging` | object | [`tbs::log::conf::Logging`](../../tobasaweb/include/tobasaweb/settings_log.h#L29) | Console and file logging settings. |

The sample files contain real configuration values, including secrets. Change
them before deployment and protect the files.

## `webapp`

| Property | JSON type | Source default | Description |
| --- | --- | --- | --- |
| `environment` | string | `"development"` for the option type | Appears in the JSON files, but the active database profile is selected by `webapp.dbConnection.environment`. |
| `dbConnection` | object | none | Database profiles and SQL logging options. |
| `httpServer` | object | `Server` defaults | HTTP and HTTPS listener settings. |
| `webService` | object | `WebService` defaults | Sessions, authentication, routes, JWT, and application directories. |
| `dbConnectionPoolSize` | integer | none in the struct | Number of database connections to keep in the pool. |

The active `environment` value belongs inside `dbConnection`. The outer
`webapp.environment` value is present in the JSON files, but it is not used by
the `Webapp` option type. `DbServiceFactory` reads
`webapp.dbConnection.environment`.

### `webapp.dbConnection`

This object is [`tbs::sql::conf::ConnectorOption`](../../tobasasql/include/tobasasql/settings.h#L43). It gives you two named database profiles and lets you choose which one is active.

| Property | JSON type | Description |
| --- | --- | --- |
| `production` | object | Settings for the production database. |
| `development` | object | Settings for the development database. |
| `environment` | string | Profile to use, normally `"production"` or `"development"`. |
| `logInternalSqlQuery` | boolean | Log SQL used internally by the framework. |
| `logSqlQuery` | boolean | Log application SQL queries. |
| `securitySalt` | string | Optional connection salt. Startup replaces it with the top-level `securitySalt`. |

Each profile is a [`tbs::sql::conf::Database`](../../tobasasql/include/tobasasql/settings.h#L33) object:

| Property | JSON type | Accepted values/meaning |
| --- | --- | --- |
| `dbDriver` | string | `SQLITE`, `MYSQL`, `PGSQL`, `ODBC`, or `ADODB`. |
| `connectionString` | string | Driver-specific connection settings, without the password. Variables are replaced before use. |
| `password` | string | Database password or encrypted password, depending on the database setup. |

#### `connectionString` syntax by driver

Keep the password in the separate `password` property. Do not put it in
`connectionString`. The framework decrypts that property with
`dbConnection.securitySalt` and adds the password in the format expected by
the selected driver. An empty password is also added.

##### SQLite

SQLite uses semicolon-separated settings:

```json
"dbDriver": "SQLITE",
"connectionString": "Database=${WS_DATADIR}/tobasa_base${DBSUFFIX}.db3;OpenCreate=True;OpenMemory=False;",
"password": ""
```

| Parameter | Value | Meaning |
| --- | --- | --- |
| `Database` | path or `:memory:` | Database file. `OpenMemory=True` forces an in-memory database. |
| `OpenReadOnly` | `True` or `False` | Open the database as read-only. |
| `OpenReadWrite` | `True` or `False` | Open the database for reading and writing. |
| `OpenCreate` | `True` or `False` | Create the file when it does not exist. |
| `OpenMemory` | `True` or `False` | Use an in-memory database. |
| `Password` | added by the framework | Used by the SQLite encryption/key step. Do not add it yourself. |

Use the `Name=Value;` form shown above, without spaces around `=`. The
framework adds the decrypted password as
`Password=<decrypted-password>;`.

##### PostgreSQL

PostgreSQL uses libpq keyword/value syntax. Separate fields with spaces:

```json
"dbDriver": "PGSQL",
"connectionString": "dbname=tobasa_base${DBSUFFIX} user=tbs_user hostaddr=10.0.0.2 port=5462",
"password": "27CA998DA4C4D345BC0C86F62B7C81BA"
```

The framework adds ` password=<decrypted-password>` before calling
`PQconnectdb`. Use the normal libpq quoting rules when a value contains spaces
or special characters. Other libpq keywords can be added in the same string.

##### ODBC

ODBC uses a semicolon-separated connection string:

```json
"dbDriver": "ODBC",
"connectionString": "Driver={ODBC Driver 17 for SQL Server};Server=10.0.0.2;Database=tobasa_base${DBSUFFIX};UID=tbs_user;APP=ws_tcxx;TrustServerCertificate=Yes;",
"password": "27CA998DA4C4D345BC0C86F62B7C81BA"
```

The framework adds `Pwd=<decrypted-password>;` and calls
`SQLDriverConnect` without prompting. Use the driver name installed on the
machine. The available attributes and values come from that ODBC driver.

##### ADODB

ADODB uses an OLE DB provider connection string:

```json
"dbDriver": "ADODB",
"connectionString": "Provider=SQLNCLI11;Server=10.0.0.2;Database=tobasa_base${DBSUFFIX};Uid=tbs_user;DataTypeCompatibility=80;APP=ws_tcxx;",
"password": "27CA998DA4C4D345BC0C86F62B7C81BA"
```

On supported MSVC builds, the framework adds
`Pwd=<decrypted-password>;` and opens the connection. `Provider` selects the
installed OLE DB provider. ADODB is conditional and is not available on
non-MSVC builds.

##### MySQL/MariaDB

MySQL uses semicolon-separated settings:

```json
"dbDriver": "MYSQL",
"connectionString": "Database=tobasa_base${DBSUFFIX};User=tbs_user;Server=10.0.0.2;Port=3306;",
"password": ""
```

The framework adds `Password=<decrypted-password>;` before calling
`mysql_real_connect`.

| Parameter | Meaning |
| --- | --- |
| `Database` | Database or schema name. |
| `User` | Database user name. |
| `Server` | Host name or address. |
| `Port` | TCP port number. |
| `Password` | Added by the framework. |

Use the exact `Name=Value;` spelling because the Tobasa parser matches these
names by prefix.

##### Password handling summary

The framework adds the password like this:

| Driver | Appended form |
| --- | --- |
| `PGSQL` | ` password=<decrypted-password>` |
| `SQLITE` | `Password=<decrypted-password>;` |
| `ODBC` | `Pwd=<decrypted-password>;` |
| `ADODB` | `Pwd=<decrypted-password>;` |
| `MYSQL` | `Password=<decrypted-password>;` |

The `password` value is passed to `crypt::passwordDecrypt` with the effective
database salt. Do not assume that a plain-text password will be encrypted for
you automatically.

### `webapp.httpServer`

This object maps to [`tbs::http::conf::Server`](../../tobasaweb/include/tobasaweb/settings_http_server.h#L67).

| Property | JSON type | Default | Description |
| --- | --- | --- | --- |
| `runHttpsOnly` | boolean | `false` | Start only the HTTPS listener. |
| `http2Enabled` | boolean | `false` when HTTP/2 is compiled in | Enable HTTP/2. This setting is read only when `TOBASA_HTTP_USE_HTTP2` is defined. |
| `address` | string | `127.0.0.1` | Address where the server listens. |
| `port` | integer | `8084` | Plain HTTP port. |
| `portHttps` | integer | `8085` | HTTPS port. |
| `timeoutRead` | integer | `60` seconds | Maximum time to read a request. |
| `timeoutWrite` | integer | `60` seconds | Maximum time to send a response. |
| `timeoutProcessing` | integer | `120` seconds | Maximum time for request processing. |
| `readBufferSize` | integer | `65536` bytes | Size of the read buffer. |
| `sendBufferSize` | integer | `65536` bytes | Size of the send buffer. |
| `maxHeaderSize` | integer | `65536` bytes | Largest accepted HTTP header block. |
| `docRoot` | string | `./wwwroot` | Directory for files served from disk. Do not add a trailing slash. |
| `temporaryDir` | string | empty, then platform temporary directory | Temporary directory for multipart requests. Relative paths use the executable directory. |
| `tls` | object | `Tls` defaults | Certificate and private-key settings. |
| `compression` | object | `Compression` defaults | Response compression settings. |
| `ioPoolSize` | integer | `4` | Number of I/O threads. `0` disables this pool. |
| `workerPoolSize` | integer | `4` | Number of request worker threads. `0` disables this pool. |
| `logVerbose` | boolean | `false` | Write detailed HTTP logs. |
| `logVerboseHttp2` | boolean | `false` when HTTP/2 is compiled in | Write detailed HTTP/2 logs. |
| `useRateLimiter` | boolean | `false` | Turn on request limiting. |
| `rateLimiterMaxRequests` | integer | `10` | Requests allowed in one window. |
| `rateLimiterWindowDuration` | integer | `1000` ms | Length of the rate-limit window. |
| `rateLimiterBlockDuration` | integer | `30000` ms | How long to block after too many violations. |
| `rateLimiterMaxViolations` | integer | `3` | Violations allowed before blocking. |
| `maxRequestsPerConnection` | integer | `100` | Maximum requests on one connection. `0` means unlimited in the runtime profile. |
| `enableMultipartParsing` | boolean | `true` | Parse multipart form and upload requests. |

#### `webapp.httpServer.tls`

This object maps to [`tbs::http::conf::Tls`](../../tobasaweb/include/tobasaweb/settings_http_server.h#L31).

| Property | JSON type | Default | Description |
| --- | --- | --- | --- |
| `certificateChainFile` | string | `./localhost.crt` | Path to the server certificate chain. |
| `privateKeyFile` | string | `./localhost.key` | Path to the server private key. |
| `password` | string | empty | Password for the private key. |
| `tmpDhFile` | string | `./dh2048.pem` | Path to the temporary DH parameter file. |
| `hostCertificates` | array | empty | Certificates for specific hostnames. |

Each `hostCertificates` entry has:

| Property | JSON type | Description |
| --- | --- | --- |
| `hostname` | string | Hostname selected by TLS SNI. |
| `certificateChainFile` | string | Certificate chain path. |
| `privateKeyFile` | string | Private key path. |
| `password` | string | Private-key password. |

TLS paths are resolved relative to the executable while `loadConfig` runs.

#### `webapp.httpServer.compression`

| Property | JSON type | Default | Description |
| --- | --- | --- | --- |
| `enable` | boolean | `true` | Turn response compression on or off. |
| `minimalLength` | integer | `1024` bytes | Compress responses at least this large. |
| `encoding` | string | `gzip` | Compression format. |
| `mimetypes` | string | selected text/JSON types | Space-separated MIME types that may be compressed. |

### `webapp.webService`

This object maps to [`tbs::web::conf::WebService`](../../tobasaweb/include/tobasaweb/settings_webapp.h).

| Property | JSON type | Default | Description |
| --- | --- | --- | --- |
| `routeAuthLists` | object | none | Extra authentication rules for route paths. |
| `sessionExpirationMinutes` | integer | `15` | Session lifetime. `0` means the session ends when the browser closes. |
| `sessionSavePath` | string | `./appdata/session` | Directory for session files. Do not add a trailing slash. |
| `acceptedClientAppId` | string | `TBSRESTC_DEV,TBSRESTC_TOBASA` | Comma-separated client application IDs accepted by the service. |
| `authJwtIssuer` | string | `TBS_WEBSVC` | Issuer written into JWTs. |
| `authJwtSecret` | string | built-in sample secret | Secret used for access tokens. Replace it for deployment. |
| `authJwtSecretRefresh` | string | built-in sample secret | Secret used for refresh tokens. Replace it for deployment. |
| `authJwtExpireTimeSpanMinutes` | integer | `15` | Access-token lifetime. |
| `authJwtRefreshExpireTimeSpanMinutes` | integer | `1440` | Refresh-token lifetime. |
| `useInMemoryResources` | boolean | `true` | Use compiled templates and static files when the executable contains them. The app forces this to `false` when the build has no embedded resources. |
| `templateDir` | string | `./views` | Directory containing templates. |
| `uploadDir` | string | `./appdata/upload` | Directory for uploaded files. Do not add a trailing slash. |
| `dataDir` | string | `./appdata` | Application data directory. Do not add a trailing slash. |
| `homePage` | string | `/dashboard` | Page opened as the application home page. |
| `loginPage` | string | `/login` | Login page route. |
| `logoutPage` | string | `/logout` | Logout route. |
| `noSessionList` | array | empty | Paths that should not create or use a session. |

#### `routeAuthLists`

| Property | JSON type | Description |
| --- | --- | --- |
| `noAuthenticationList` | array | Paths that should not require authentication. |
| `needAuthenticationList` | array | Paths that should require authentication. |

Each entry has `path`, `check`, and `authScheme`. The checked-in examples use
`starts_with` and the `bearer` scheme. Use a `check` value supported by the
router, otherwise the rule may not match as expected.

#### `noSessionList`

Each entry has:

| Property | JSON type | Description |
| --- | --- | --- |
| `path` | string | Path or path part to match. |
| `check` | string | Matching operation, such as `ends_with`. |

## `logging`

The `logging` object maps to [`tbs::log::conf::Logging`](../../tobasaweb/include/tobasaweb/settings_log.h#L29).

| Property | JSON type | Description |
| --- | --- | --- |
| `multiSinkLevel` | string | Overall minimum level for the logging system. |
| `stdoutColor` | object | Colored console output settings. |
| `fileSink` | object | Main log-file settings. |
| `fileSinkD` | object | Optional detailed/debug log-file settings. |

Log levels are `trace`, `debug`, `info`, `warn`, `error`, `critical`, `off`,
or `n_level`, matching the `spdlog` enum conversion.

Each sink object has:

| Property | JSON type | Description |
| --- | --- | --- |
| `level` | string | Lowest level written by the sink. |
| `pattern` | string | `spdlog` output format. |
| `filePath` | string | Log-file path. Used by file sinks; variables are expanded. |
| `truncate` | boolean | Clear the file when it is opened. Used by file sinks. |
| `enable` | boolean | Turn a file sink on or off. Used by file sinks. |

`stdoutColor` uses only `level` and `pattern`. `fileSink` and `fileSinkD` use
all five properties. The checked-in configuration enables `fileSink` and
disables `fileSinkD`.

## Separate header-rule configuration

The application loads `configuration/appsettings_header_rules.json` separately
after the main configuration. The embedded copy is used in the same way as
the embedded main configuration. The loaded option is stored as
`httpResponseHeaderRule` and uses `HttpResponseHeaderRule`.

### `httpResponseHeaderRule`

| Property | JSON type | Description |
| --- | --- | --- |
| `cacheControl` | array | Cache-control rules selected by host and request path. |
| `headerRule` | array | Response-header and CORS rules. |

Each `cacheControl` item has `host`, `requestPath`, and `rules`. Each rule
contains a regular-expression `pattern` and the header `value` to send.

Each `headerRule` item has:

| Property | JSON type | Description |
| --- | --- | --- |
| `type` | string | `none` for normal headers or `cors` for CORS headers. |
| `hostOrigin` | array of strings | Hosts, or allowed origins when `type` is `cors`. |
| `requestPath` | array of strings | Paths where the rule applies. |
| `headers` | array | Header objects with string `key` and `value`. |
