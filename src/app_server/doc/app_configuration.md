# Application configuration

The application configuration is JSON stored in the process-wide
[`tbs::Config`](../../tobasa/include/tobasa/config.h#L16) object. The
application server loads it once during startup. It does this before creating
the database service and before starting the HTTP server.

## Configuration files

The application server can get its main configuration from two places:

- `configuration/appsettings.json` is the file used at runtime.
- `configuration_embed/appsettings.json` is a fallback compiled into the
  executable as an application resource.

At runtime, the server looks beside the executable in
`<executable directory>/configuration/appsettings.json`. The CMake post-build
step copies the runtime file there when the source file is available.

The embedded file is created from `configuration_embed/*.json` during the
build. The application can read it with
`app::Resource::get("config/appsettings.json", "config")` even when templates
and static files are not embedded.

## Loading

Startup uses code like this:

```cpp
auto configFile = app::configDir() + path::SEPARATOR + "appsettings.json";
auto embeddedConfig = app::Resource::get("config/appsettings.json", "config");
webapp.loadConfig(configFile, embeddedConfig);
```

`Webapp::loadConfig` calls `Config::load(configFile, embeddedConfig)`. The
loader uses this order:

1. Open and parse `configFile`.
2. If the file cannot be opened and embedded data is available, parse the
   embedded data.
3. If neither source can be parsed, report the error and stop startup.

This is fallback behavior, not a merge. A runtime file with missing settings
does not receive those settings from the embedded file. The one JSON document
that was selected becomes the configuration object in `_jsonConf`.

After a successful parse, `Config::valid()` is `true`. The parser accepts
comments in JSON. Parse errors, missing required data, and other configuration
problems are reported by `Webapp::loadConfig`, which returns `false`. The
application server then exits without starting.

## Embedded configuration

The embedded file is useful when you want a self-contained executable or a
known fallback configuration. It is not an extra layer of defaults at runtime.
To change it, edit the matching file under `configuration_embed/` and rebuild.

The main embedded file normally contains:

- `configVariables`: replacements used while reading string values;
- `securitySalt`: the application-wide salt for authentication and database
  password operations;
- `webapp`: database, HTTP server, and web-service settings;
- `logging`: console and file logger settings.

The separate `configuration_embed/appsettings_header_rules.json` resource is
loaded after the main configuration:

```cpp
[`Config::addOption`](../../tobasa/include/tobasa/config.h#L104)<web::conf::HttpResponseHeaderRule>(
	 "httpResponseHeaderRule", headerRuleFile, embeddedHeaderRule);
```

It also tries the file first and then the embedded copy. It is stored under
`httpResponseHeaderRule`; it is not merged into the main JSON document.

## Runtime configuration

Edit the deployed file beside the executable:

```text
<executable directory>/configuration/appsettings.json
```

When this file can be opened and parsed, it is used instead of the embedded
copy. If you remove or rename it, the embedded file is used at the next
startup.

Configuration is read only during startup. Editing the file while the server
is running does not reload it.

The main objects can be read in C++ like this:

```cpp
auto webapp = Config::getOption<web::conf::Webapp>("webapp");
auto logging = Config::getOption<log::conf::Logging>("logging");
```

You can read a nested value by using a dotted path:

```cpp
auto port = Config::getNestedOption<int>("webapp.httpServer.port");
```

Use `tryGetNestedOption(path, defaultValue)` when a missing key or wrong type
should use a value you provide.

`getOption` throws when the global configuration is invalid. If a top-level
option is missing, it logs an error and returns a default-constructed option.
`getNestedOption` throws when the path is missing or cannot be converted to the
requested type.

### Variable substitution

`configVariables` contains string-to-string replacements. The loader checks
every string value in the main file for placeholders such as
`${WS_DATADIR}` and `${DBSUFFIX}`.

Every placeholder must have a matching key. You can use several placeholders
in one value and several copies of the same placeholder.

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
environment variables. They are not filled from the process environment.

### Startup adjustments

After reading the JSON, `Webapp::loadConfig` copies the top-level
`securitySalt` into `webapp.dbConnection.securitySalt`:

```cpp
Config::setNestedOption("webapp.dbConnection.securitySalt", globalSalt);
```

It also makes temporary-directory and TLS paths relative to the executable
when needed. The temporary directory is created if it does not exist.

The normalized path values are applied to the deserialized `Webapp` option;
they are not written back to `_jsonConf`. The security-salt assignment is
written to `_jsonConf`, so it can be seen through `getConfiguration()`.

`useInMemoryResources` is also copied to the static
`web::conf::Webapp::useInMemoryResources` flag. If the executable was not
built with embedded resources, the application forces this flag to `false`.

## Dumping the effective configuration

The configuration actually held in memory is available through
[`Config::getConfiguration`](../../tobasa/include/tobasa/config.h#L213):

```cpp
const Json& effective = tbs::Config::get().getConfiguration();
std::cout << effective.dump(3) << '\n';
```

This object includes the values loaded from the selected file and the changes
made during startup. It is the best object to inspect when debugging. Reading
the runtime file again may not show those startup changes.

### Effective configuration structure

When the application is ready to start, the effective configuration has this
shape. The values below show types, not real passwords or deployment values:

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

`RouteAuth` entries have `path`, `check`, and `authScheme`. `RouteSession`
entries have `path` and `check`. `KeyValue` entries have `key` and `value`.
`LogLevel` can be `trace`, `debug`, `info`, `warn`, `error`, `critical`, `off`,
or `n_level`.

This tree shows the keys in the JSON returned by `getConfiguration()`. It is
not limited to fields copied into typed option objects.

In particular, `webapp.environment` stays in the JSON loaded from the checked-
in `appsettings.json`, but `tbs::web::conf::Webapp` does not declare or use
it. The active database profile comes from
`webapp.dbConnection.environment`.

When the LIS module is compiled and starts successfully, the application adds
another top-level `lisEngine` object through `Config::addOption`. Its shape is
defined by `tbs::lis::conf::Engine` and comes from `appsettings_lis.json`.
The object is absent when the module is disabled or its configuration fails.

The application server also returns this object from
`GET /api/running_configuration`. The route requires bearer authentication,
and the authenticated user must also be named `admin`.

The response looks like this:

```json
{
	"config": {
		"webapp": {},
		"logging": {}
	}
}
```

Treat this response as sensitive. It can contain database connection details,
passwords, TLS passwords, the application security salt, and JWT secrets. Do
not expose this endpoint publicly or write its output to an open log.
