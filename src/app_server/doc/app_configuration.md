# Application configuration

The application configuration is JSON loaded into the process-wide
[`tbs::Config`](../../tobasa/include/tobasa/config.h#L16) singleton. The application server loads its main configuration
once during startup, before it creates the database service and starts the
HTTP server.

## Configuration files

The application server has two sources for its main configuration:

- `configuration/appsettings.json` is the runtime configuration file.
- `configuration_embed/appsettings.json` is the embedded fallback compiled
	into the executable as an application resource.

The path used by the application server is the executable directory followed
by `configuration/appsettings.json` (`app::configDir()`). The runtime file is
copied there by the CMake post-build step when the source configuration files
exist.

The embedded file is generated from `configuration_embed/*.json` at build
time. It is available through `app::Resource::get("config/appsettings.json",
"config")`, regardless of whether templates and static files are built into
memory.

## Loading

Startup calls:

```cpp
auto configFile = app::configDir() + path::SEPARATOR + "appsettings.json";
auto embeddedConfig = app::Resource::get("config/appsettings.json", "config");
webapp.loadConfig(configFile, embeddedConfig);
```

`Webapp::loadConfig` delegates to `Config::load`(configFile,
embeddedConfig). `Config::load` uses this order:

1. Open and parse the file at `configFile`.
2. If the file cannot be opened and `embeddedConfig` is non-empty, parse the
	 embedded bytes instead.
3. If neither source can be parsed, report a configuration error and fail
	 startup.

This is fallback behavior, not a merge. A partially populated runtime file is
not supplemented with missing keys from the embedded file. The selected JSON
document becomes the singleton's `_jsonConf` object and `Config::valid()` is
set to `true` after a successful parse.

The parser accepts JSON comments. Parse errors, missing required data, and
other configuration errors are reported by `Webapp::loadConfig`, which
returns `false`; the application server exits without starting.

## Embedded configuration

Embedded configuration is useful for a self-contained executable and as a
last-known default. It is not a second layer of defaults at runtime. To
change the embedded fallback, edit the corresponding file under
`configuration_embed/` and rebuild the application.

The main embedded file normally contains these top-level objects:

- `configVariables`: substitutions used while parsing string values.
- `securitySalt`: the application-wide salt used by authentication and
	database password operations.
- `webapp`: database, HTTP server, and web-service settings.
- `logging`: stdout and file logger settings.

The separate `configuration_embed/appsettings_header_rules.json` resource is
loaded after the main configuration with:

```cpp
[`Config::addOption`](../../tobasa/include/tobasa/config.h#L104)<web::conf::HttpResponseHeaderRule>(
	 "httpResponseHeaderRule", headerRuleFile, embeddedHeaderRule);
```

It is also file-first with embedded fallback, but is added under the
`httpResponseHeaderRule` key rather than merged into the main JSON document.

## Runtime configuration

Edit the deployed file beside the executable:

```text
<executable directory>/configuration/appsettings.json
```

The runtime file wins whenever it can be opened and parsed. Removing or
renaming it makes the application use the embedded `appsettings.json` on the
next startup. Configuration is loaded at startup; editing the file while the
server is running does not reload it.

The main settings are deserialized from the JSON object at the point where
they are used:

```cpp
auto webapp = Config::getOption<web::conf::Webapp>("webapp");
auto logging = Config::getOption<log::conf::Logging>("logging");
```

Nested values can be read by dotted path:

```cpp
auto port = Config::getNestedOption<int>("webapp.httpServer.port");
```

Use `tryGetNestedOption(path, defaultValue)` when a missing key or a type
mismatch should use a fallback. `getOption` throws when the global
configuration is invalid; a missing top-level option logs an error and
returns a default-constructed option object. `getNestedOption` throws when a
path is missing or cannot be converted.

### Variable substitution

`configVariables` is an object whose keys and values are strings. Every string
value in the main `appsettings.json` is scanned for placeholders such as
`${WS_DATADIR}` and `${DBSUFFIX}`. The placeholder must match a key in
`configVariables`; an undefined placeholder raises a configuration error.
Multiple placeholders in one value are supported.

For example:

```json
{
	"configVariables": {
		"${WS_DATADIR}": "./appdata"
	},
	"webapp": {
		"httpServer": {
			"temporaryDir": "${WS_DATADIR}/tmp"
		}
	}
}
```

These are application configuration variables, not operating-system
environment variables. They are not automatically populated from the
process environment.

### Startup adjustments

After parsing, `Webapp::loadConfig` copies the top-level `securitySalt` into
`webapp.dbConnection.securitySalt` with:

```cpp
Config::setNestedOption("webapp.dbConnection.securitySalt", globalSalt);
```

It also normalizes the configured temporary directory and TLS certificate
paths relative to the executable, creating the temporary directory when
necessary. Those path changes are applied to `Webapp`'s deserialized option;
they are not written back to `_jsonConf`. The security-salt assignment above
does update `_jsonConf` and is consequently visible through
`getConfiguration()`.

The `useInMemoryResources` setting is additionally applied to the static
`web::conf::Webapp::useInMemoryResources` flag. When the executable was not
built with in-memory resources, the application forces that flag to `false`.

## Dumping the effective configuration

The authoritative in-memory JSON object is available through
[`Config::getConfiguration`](../../tobasa/include/tobasa/config.h#L213):

```cpp
const Json& effective = tbs::Config::get().getConfiguration();
std::cout << effective.dump(3) << '\n';
```

`getConfiguration()` returns the object after parsing and after configuration
mutations made during startup. It is therefore the right object to inspect
when diagnosing what the process is using, rather than rereading the runtime
file.

### Effective configuration structure

At the point where the application server is ready to start, the effective
configuration has this JSON structure. Values are shown as types rather than
secrets or deployment-specific values:

```text
root
|- configVariables: object<string, string>
|- securitySalt: string
|- webapp: object
|  |- environment: string                 # retained JSON field; not part of Webapp
|  |- dbConnection: object
|  |  |- production: Database
|  |  |  |- dbDriver: "SQLITE" | "MYSQL" | "PGSQL" | "ODBC" | "ADODB"
|  |  |  |- connectionString: string
|  |  |  `- password: string
|  |  |- development: Database
|  |  |  |- dbDriver: "SQLITE" | "MYSQL" | "PGSQL" | "ODBC" | "ADODB"
|  |  |  |- connectionString: string
|  |  |  `- password: string
|  |  |- environment: "development" | "production"
|  |  |- logInternalSqlQuery: boolean
|  |  |- logSqlQuery: boolean
|  |  `- securitySalt: string       # set from root.securitySalt during startup
|  |- dbConnectionPoolSize: integer
|  |- httpServer: object
|  |  |- runHttpsOnly: boolean
|  |  |- http2Enabled: boolean     # present when built with HTTP/2 support
|  |  |- address: string
|  |  |- port: integer
|  |  |- portHttps: integer
|  |  |- timeoutRead: integer
|  |  |- timeoutWrite: integer
|  |  |- timeoutProcessing: integer
|  |  |- readBufferSize: integer
|  |  |- sendBufferSize: integer
|  |  |- maxHeaderSize: integer
|  |  |- docRoot: string
|  |  |- temporaryDir: string
|  |  |- tls: object
|  |  |  |- certificateChainFile: string
|  |  |  |- privateKeyFile: string
|  |  |  |- password: string
|  |  |  |- tmpDhFile: string
|  |  |  `- hostCertificates: array<object>
|  |  |     `- hostname, certificateChainFile, privateKeyFile, password: string
|  |  |- compression: object
|  |  |  |- enable: boolean
|  |  |  |- minimalLength: integer
|  |  |  |- encoding: string
|  |  |  `- mimetypes: string
|  |  |- ioPoolSize: integer
|  |  |- workerPoolSize: integer
|  |  |- logVerbose: boolean
|  |  |- logVerboseHttp2: boolean  # present when built with HTTP/2 support
|  |  |- useRateLimiter: boolean
|  |  |- rateLimiterMaxRequests: integer
|  |  |- rateLimiterWindowDuration: integer
|  |  |- rateLimiterBlockDuration: integer
|  |  |- rateLimiterMaxViolations: integer
|  |  |- maxRequestsPerConnection: integer
|  |  `- enableMultipartParsing: boolean
|  `- webService: object
|     |- routeAuthLists: object
|     |  |- noAuthenticationList: array<RouteAuth>
|     |  `- needAuthenticationList: array<RouteAuth>
|     |- sessionExpirationMinutes: integer
|     |- sessionSavePath: string
|     |- acceptedClientAppId: string
|     |- authJwtIssuer: string
|     |- authJwtSecret: string
|     |- authJwtSecretRefresh: string
|     |- authJwtExpireTimeSpanMinutes: integer
|     |- authJwtRefreshExpireTimeSpanMinutes: integer
|     |- useInMemoryResources: boolean
|     |- templateDir: string
|     |- uploadDir: string
|     |- dataDir: string
|     |- homePage: string
|     |- loginPage: string
|     |- logoutPage: string
|     `- noSessionList: array<RouteSession>
|- logging: object
|  |- multiSinkLevel: LogLevel
|  |- stdoutColor: LogStdout
|  |  |- level: LogLevel
|  |  `- pattern: string
|  |- fileSink: LogBasicFile
|  |  |- level: LogLevel
|  |  |- pattern: string
|  |  |- filePath: string
|  |  |- truncate: boolean
|  |  `- enable: boolean
|  `- fileSinkD: LogBasicFile
`- httpResponseHeaderRule: object
	|- cacheControl: array<CacheControlItem>
	|  `- host, requestPath: array<string>; rules: array<PatternHeader>
	|     `- pattern, header: string
	`- headerRule: array<HeaderRuleItem>
		`- type: string; hostOrigin, requestPath: array<string>;
			headers: array<KeyValue>
```

`RouteAuth` entries contain `path`, `check`, and `authScheme` strings.
`RouteSession` entries contain `path` and `check` strings. `KeyValue` entries
contain `key` and `value` strings. `LogLevel` is one of `trace`, `debug`,
`info`, `warn`, `error`, `critical`, `off`, or `n_level`.

The tree describes keys in the JSON returned by `getConfiguration()`, not
only fields that are copied into a typed option. In particular,
`webapp.environment` remains in the JSON loaded from either checked-in
`appsettings.json`, but `tbs::web::conf::Webapp` does not declare or use it.
The active database profile is selected by
`webapp.dbConnection.environment`.

When the LIS module is compiled and successfully initialized, an additional
top-level `lisEngine` object is inserted by `Config::addOption`. Its shape is
defined by `tbs::lis::conf::Engine` and comes from the separate
`appsettings_lis.json` file; it is absent when the module is not enabled or
its configuration fails to load.

The application server exposes the same object through `GET
/api/running_configuration`. That route requires bearer authentication and
then additionally requires the authenticated user to be named `admin`.
The response has the form:

```json
{
	"config": {
		"webapp": {},
		"logging": {}
	}
}
```

Treat this dump as sensitive. It can include database connection details,
passwords, TLS passwords, the application security salt, and JWT secrets.
Do not publish the endpoint or write its output to an unrestricted log.


