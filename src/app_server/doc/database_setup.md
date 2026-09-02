# Application database setup

The application can initialize its database schema automatically. The quickest
way to run the server locally is SQLite: configure a database file path and
let the application create the file, tables, views, and initial content on
first startup.

## Quick start with SQLite

1. Open the deployed runtime configuration:

	```text
	<executable directory>/configuration/appsettings.json
	```

2. Set the active database profile to `development` and configure its SQLite
	connection:

	```json
	"webapp": {
	  "dbConnection": {
		 "environment": "development",
		 "development": {
			"dbDriver": "SQLITE",
			"connectionString": "Database=./appdata/tobasa_base.db3;OpenCreate=True;OpenMemory=False;",
			"password": ""
		 }
	  },
	  "dbConnectionPoolSize": 4
	}
	```

	The repository configuration uses the equivalent placeholder form:

	```json
	"connectionString": "Database=${WS_DATADIR}/tobasa_base${DBSUFFIX}.db3;OpenCreate=True;OpenMemory=False;"
	```

	with `${WS_DATADIR}` set to `./appdata` and `${DBSUFFIX}` set to an empty
	string in the embedded configuration.

3. Start the server. With `OpenCreate=True`, the SQLite backend opens the
	configured file for read/write and creates it when it does not exist.

The database profile selected by the application is
`webapp.dbConnection.environment`. The outer `webapp.environment` property
present in the sample JSON is not a member of the typed `Webapp` option and
does not select the database profile.

The application appends the database password to the connection string after
loading it. For SQLite the final form is
`Password=<decrypted-password>;`. Keep the password in the profile's
`password` property rather than adding `Password` to `connectionString`.

### SQLite connection options

The SQLite connection implementation recognizes these semicolon-delimited
parameters:

| Parameter | Meaning |
| --- | --- |
| `Database` | SQLite file path. |
| `OpenCreate=True` | Open read/write and create the file if missing. |
| `OpenReadOnly=True` | Open read-only. |
| `OpenReadWrite=True` | Open read/write. |
| `OpenMemory=True` | Use `:memory:` instead of the configured file path. |

The application also enables SQLite foreign keys, a busy timeout, and WAL mode
after a successful connection. These are runtime connection settings and do
not need to be added to `connectionString`.

## Using another database

For PostgreSQL, MySQL/MariaDB, ODBC, or ADODB:

1. Install and configure the required client library/driver for the build.
2. Create an empty database using the database server or administration tool.
3. Select the matching `dbDriver` and put the driver-specific connection
	details in the appropriate `production` or `development` profile.
4. Set `webapp.dbConnection.environment` to that profile name.
5. Set the profile's `password` value and ensure the application security salt
	is available. The application uses that salt to decrypt the password before
	connecting.
6. Start the server. The application creates the migration bookkeeping table,
	builds the schema for the selected driver, and inserts the base default
	content.

The database should be empty from the application's point of view. Do not
pre-create the Tobasa tables or `schema_migrations`; the migration code creates
them and records what it has applied. The database account must be allowed to
create tables, views, constraints, and indexes, and to insert the initial
content.

### Driver examples

These examples are taken from the runtime `configuration/appsettings.json`.
The framework appends the decrypted password as described below; the samples
therefore leave credentials out of `connectionString`.

#### PostgreSQL

```json
"dbDriver": "PGSQL",
"connectionString": "dbname=tobasa_base${DBSUFFIX} user=tbs_user hostaddr=10.0.0.2 port=5462",
"password": "27CA998DA4C4D345BC0C86F62B7C81BA"
```

The string is passed to libpq and the framework appends
` password=<decrypted-password>`.

#### MySQL/MariaDB

```json
"dbDriver": "MYSQL",
"connectionString": "Database=tobasa_base${DBSUFFIX};User=tbs_user;Server=10.0.0.2;Port=3306;",
"password": ""
```

The Tobasa MySQL connection reads `Database`, `User`, `Server`, and numeric
`Port`. The framework appends `Password=<decrypted-password>;` before calling
`mysql_real_connect`.

#### ODBC

```json
"dbDriver": "ODBC",
"connectionString": "Driver={ODBC Driver 17 for SQL Server};Server=10.0.0.2;Database=tobasa_base${DBSUFFIX};UID=tbs_user;APP=ws_tcxx;TrustServerCertificate=Yes;",
"password": "27CA998DA4C4D345BC0C86F62B7C81BA"
```

The framework appends `Pwd=<decrypted-password>;` and passes the resulting
string to `SQLDriverConnect` with `SQL_DRIVER_NOPROMPT`. The `Driver` name
must match an installed ODBC driver.

#### ADODB

```json
"dbDriver": "ADODB",
"connectionString": "Provider=SQLNCLI11;Server=10.0.0.2;Database=tobasa_base${DBSUFFIX};Uid=tbs_user;DataTypeCompatibility=80;APP=ws_tcxx;",
"password": "27CA998DA4C4D345BC0C86F62B7C81BA"
```

On supported MSVC builds, the framework appends `Pwd=<decrypted-password>;`
and passes the result to `ADODB::Connection::Open`. ADODB is conditional on
MSVC and the corresponding build option.

## What happens on first startup

The server performs database setup before starting the HTTP server:

1. `Webapp` registers the base migration `001`.
2. The application selects `production` unless
	`webapp.dbConnection.environment` is exactly `development`; that selects
	the development profile.
3. It connects using the selected driver and profile.
4. It creates `schema_migrations` if necessary. The table has `version`,
	`module_name`, and `note`, with `(version, module_name)` as its primary key.
5. It runs each registered migration that is not already recorded.
6. If all startup checks complete, the web application starts and uses the
	same database service.

If the database connection cannot be opened, migration checking is skipped and
the error is logged. Startup may subsequently fail its database connectivity
check; a successful migration is not assumed when the connection failed.

## Base migration `001`

`001` is registered for every `Webapp`. Its driver-specific schema creates the
base application tables and views. The table names include:

- `base_users`
- `base_roles`
- `base_sites`
- `base_user_role`
- `base_user_site`
- `base_users_reset_password`
- `base_acl`
- `base_auth_log`
- `base_class_code`
- `base_menu`
- `company`
- `base_event_log`
- `base_app_task`

It also creates the base views `v_base_acl`, `v_base_menu_group`,
`v_base_menu_type`, `v_base_menu`, `v_base_user_roles`, and
`v_base_user_site`.

The base schema migration populates default class codes, menus, company/site
and role data, and the initial user records defined in the driver-specific
schema. It then creates ACL entries for the user role (role ID `2`) for the
base menus. The exact column types and defaults are driver-specific; the
source contains separate SQLite, PostgreSQL, MySQL, and MSSQL schema scripts.

## Optional module migrations

Additional migrations are registered only when the corresponding build/module
is enabled:

| Version | Module name | Condition | Content |
| --- | --- | --- | --- |
| `002` | `BASE` | `TOBASA_USE_TESTS_MODULE` | Adds the test menu group, WebSocket test menus, and ACL entries for role ID `2`. |
| `003` | `BASE` | `TOBASA_USE_LIS_ENGINE` | Adds LIS class/menu data, the LIS user role (role ID `5`), and LIS ACL entries. |

The LIS migration is registered by the LIS module and is run through the same
`schema_migrations` mechanism. It is not applied merely because a database
contains a previous application version; the LIS module must be built and
initialized.

## How migrations are applied

For each registered migration, the application queries
`schema_migrations` for its `(version, module_name)` pair. If the pair is
absent, it:

1. Begins a transaction.
2. Executes the migration's `up()` statements.
3. Inserts the migration version, module name, and note into
	`schema_migrations`.
4. Commits the transaction.

If a migration fails, the application rolls back that migration and does not
insert its bookkeeping row. The next startup will try it again. Already
recorded migrations are skipped, so restarting the application does not
recreate the schema or duplicate the default content.

Migration order is the order in which migrations are registered: base `001`,
then test `002` when enabled, then LIS `003` when enabled. Migration records
are identified by both version and module name, not by version alone.

## Troubleshooting

- Verify that `webapp.dbConnection.environment` names the intended profile.
- Verify that `dbDriver` matches the connection-string syntax and an enabled
  backend in the build.
- For SQLite, verify the parent directory exists and the process can write it;
  `OpenCreate=True` creates the database file, not missing parent directories.
- For server databases, verify the database already exists and the account can
  create schema objects.
- Check the application log for connection or migration errors.
- Inspect `schema_migrations` to see which migration versions completed.
- Do not delete individual migration rows from a populated database unless you
  understand that the corresponding migration will be attempted again.
