# Application database setup

The application can create and update its database schema during startup. For
local development, SQLite is the easiest choice: set a file path and let the
application create the database, tables, views, and initial data.

## Quick start with SQLite

1. Open the runtime configuration next to the executable:

   ```text
   <executable directory>/configuration/appsettings.json
   ```

2. Select the `development` profile and set its SQLite connection:

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

   The repository configuration uses the same settings with variables:

   ```json
   "connectionString": "Database=${WS_DATADIR}/tobasa_base${DBSUFFIX}.db3;OpenCreate=True;OpenMemory=False;"
   ```

   In the embedded configuration, `${WS_DATADIR}` is `./appdata` and
   `${DBSUFFIX}` is empty.

3. Start the server. With `OpenCreate=True`, SQLite opens the file for
   reading and writing and creates it when it is missing.

The profile selected by the application is
`webapp.dbConnection.environment`. The sample JSON also has an outer
`webapp.environment`, but that field is not part of the typed `Webapp` option
and does not select the database profile.

The application adds the database password after it loads the profile. For
SQLite, the final connection string contains
`Password=<decrypted-password>;`. Put the password in the profile's
`password` property. Do not add `Password` yourself to `connectionString`.

### SQLite connection options

SQLite accepts these semicolon-separated settings:

| Parameter | Meaning |
| --- | --- |
| `Database` | Path to the SQLite file. |
| `OpenCreate=True` | Open for writing and create the file when it is missing. |
| `OpenReadOnly=True` | Open the database as read-only. |
| `OpenReadWrite=True` | Open the database for reading and writing. |
| `OpenMemory=True` | Use `:memory:` instead of the configured file. |

After connecting, the application also enables foreign keys, a busy timeout,
and WAL mode. You do not need to add those settings to `connectionString`.

## Using another database

For PostgreSQL, MySQL/MariaDB, ODBC, or ADODB:

1. Install the client library or driver required by your build.
2. Create an empty database with the database server or its administration
   tool.
3. Set the matching `dbDriver` and add the driver-specific connection
   settings to the `production` or `development` profile.
4. Set `webapp.dbConnection.environment` to the profile you want to use.
5. Set the profile's `password` and make sure the application security salt is
   available. The application uses the salt to decrypt the password.
6. Start the server. It creates the migration table, applies the schema for
   the selected driver, and adds the base data.

Start with an empty database from the application's point of view. Do not
create Tobasa tables or `schema_migrations` yourself. The migration code
creates them and records what it has applied.

The database account must be allowed to create tables, views, constraints, and
indexes, and to insert the initial data.

### Driver examples

These examples come from the runtime `configuration/appsettings.json`. The
framework adds the decrypted password later, so keep credentials out of
`connectionString`.

#### PostgreSQL

```json
"dbDriver": "PGSQL",
"connectionString": "dbname=tobasa_base${DBSUFFIX} user=tbs_user hostaddr=10.0.0.2 port=5462",
"password": "27CA998DA4C4D345BC0C86F62B7C81BA"
```

The string is passed to libpq. The framework adds
` password=<decrypted-password>`.

#### MySQL/MariaDB

```json
"dbDriver": "MYSQL",
"connectionString": "Database=tobasa_base${DBSUFFIX};User=tbs_user;Server=10.0.0.2;Port=3306;",
"password": ""
```

The Tobasa MySQL connection reads `Database`, `User`, `Server`, and numeric
`Port`. The framework adds
`Password=<decrypted-password>;` before calling `mysql_real_connect`.

#### ODBC

```json
"dbDriver": "ODBC",
"connectionString": "Driver={ODBC Driver 17 for SQL Server};Server=10.0.0.2;Database=tobasa_base${DBSUFFIX};UID=tbs_user;APP=ws_tcxx;TrustServerCertificate=Yes;",
"password": "27CA998DA4C4D345BC0C86F62B7C81BA"
```

The framework adds `Pwd=<decrypted-password>;` and passes the result to
`SQLDriverConnect` with `SQL_DRIVER_NOPROMPT`. The `Driver` value must match a
driver installed on the machine.

#### ADODB

```json
"dbDriver": "ADODB",
"connectionString": "Provider=SQLNCLI11;Server=10.0.0.2;Database=tobasa_base${DBSUFFIX};Uid=tbs_user;DataTypeCompatibility=80;APP=ws_tcxx;",
"password": "27CA998DA4C4D345BC0C86F62B7C81BA"
```

On supported MSVC builds, the framework adds
`Pwd=<decrypted-password>;` and passes the result to
`ADODB::Connection::Open`. ADODB support depends on MSVC and the matching
build option.

## What happens on first startup

Database setup runs before the HTTP server starts:

1. `Webapp` registers base migration `001`.
2. The application uses `production` unless
   `webapp.dbConnection.environment` is exactly `development`.
3. It connects with the selected driver and profile.
4. It creates `schema_migrations` if needed. The table stores `version`,
   `module_name`, and `note`, with `(version, module_name)` as its primary key.
5. It runs each registered migration that is not already recorded.
6. When the startup checks finish, the web application starts with the same
   database service.

If the connection cannot be opened, migration checking is skipped and the
error is logged. A later database connectivity check may still stop startup.
A failed connection is not treated as a successful migration.

## Base migration `001`

Migration `001` is registered for every `Webapp`. Its driver-specific schema
creates the main application tables and views. The table names include:

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

The migration adds default class codes, menus, company/site and role data, and
the initial users defined by the driver-specific schema. It then adds ACL
entries for role ID `2` and the base menus.

Column types and defaults depend on the database driver. The source contains
separate schema scripts for SQLite, PostgreSQL, MySQL, and MSSQL.

## Optional module migrations

These migrations are added only when the matching module is enabled:

| Version | Module name | Condition | Content |
| --- | --- | --- | --- |
| `002` | `BASE` | `TOBASA_USE_TESTS_MODULE` | Adds the test menu group, WebSocket test menus, and ACL entries for role ID `2`. |
| `003` | `BASE` | `TOBASA_USE_LIS_ENGINE` | Adds LIS class and menu data, the LIS user role (role ID `5`), and LIS ACL entries. |

The LIS module registers migration `003`, and it uses the same
`schema_migrations` table. A database from an older application version does
not automatically receive the LIS migration. The LIS module must be built and
initialized.

## How migrations are applied

For every registered migration, the application looks for its
`(version, module_name)` pair in `schema_migrations`. When it is not there, the
application:

1. Starts a transaction.
2. Runs the migration's `up()` statements.
3. Stores the version, module name, and note in `schema_migrations`.
4. Commits the transaction.

If a migration fails, that migration is rolled back and its bookkeeping row is
not added. The next startup tries it again.

Recorded migrations are skipped, so restarting the server does not recreate
tables or add duplicate default data.

Migrations run in registration order: base `001`, test `002` when enabled, and
LIS `003` when enabled. The pair of version and module name identifies a
migration; the version number alone is not enough.

## Troubleshooting

- Check that `webapp.dbConnection.environment` names the profile you meant to
  use.
- Check that `dbDriver` matches both the connection-string format and a
  backend enabled in the build.
- For SQLite, make sure the parent directory exists and the process can write
  to it. `OpenCreate=True` creates the database file, not missing directories.
- For a server database, make sure the database already exists and the account
  can create schema objects.
- Read the application log for connection and migration errors.
- Inspect `schema_migrations` to see which migrations completed.
- Do not remove individual migration rows from a populated database unless you
  understand that the application will try those migrations again.
