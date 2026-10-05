# Prepared statements in TobasaSQL

TobasaSQL supports SQL parameters. That means you can pass values into a query without building a raw SQL string by hand.

This library also supports a prepared-operation flow. The exact details depend on the database driver, but the overall idea is the same across backends.

## What `SqlQuery` does

`SqlQuery<Driver>` is the easy, high-level API most apps should use.

It keeps these things together:

- the SQL text,
- the parameter list,
- the active connection,
- the driver-specific command object behind it.

You can use it in one step:

```cpp
using namespace tbs::sql;

SqlConnection<SqliteDriver> conn;
conn.connect("Database=./app.db3;OpenCreate=True;");

SqlQuery<SqliteDriver> query(conn, "SELECT name FROM people WHERE id = :id");
query.addParam("id", DataType::integer, 7);

std::string name = query.executeScalar();
```

You can also prepare it yourself and run it in stages:

```cpp
SqlQuery<SqliteDriver> query(conn);
query.prepare("INSERT INTO people (id, name) VALUES (:id, :name)");
query.addParam("id", DataType::integer, 10);
query.addParam("name", DataType::varchar, std::string("Ada"));
query.execute();
```

So `SqlQuery` is not limited to only one call. It supports a normal flow like:

- prepare the SQL,
- add parameters,
- bind them,
- run the command,
- reset or close when done.

## Named parameters are rewritten automatically

By default, `SqlQuery` uses named parameters.

That means you write SQL like this:

```cpp
"SELECT * FROM users WHERE id = :id AND name = :name"
```

Before the command runs, the library rewrites it to the style that the selected backend expects.

Examples:

- MySQL / SQLite: `?`
- PostgreSQL: `$1`, `$2`, ...

If the SQL already uses the backend's native style, you can use `ParameterStyle::native` and it will not rewrite the text.

```cpp
SqlQuery<MysqlDriver> query(
   conn,
   "SELECT * FROM users WHERE id = :id AND name = :name",
   ParameterStyle::named);

query.addParam("id", DataType::integer, 1);
query.addParam("name", DataType::varchar, "Alice");
```

This is still a parameterized query. You are not concatenating values into SQL by hand.

## The driver command classes

Under the hood, each database driver has its own command class. These are the lower-level classes that do the actual prepared execution.

Examples:

- `MysqlCommand`
- `PgsqlCommand`
- `SqliteCommand`
- `OdbcCommand`
- `AdodbCommand`

They all follow a similar flow:

- `query(sql, params)`
- `prepare(sql)`
- `bind(params)`
- `execute()`, `executeScalar()`, or `executeResult()`
- `reset()`
- `close()`

So `SqlQuery` is the simple portable layer, and the driver command class is the real backend-specific implementation.

### Example: direct driver command usage

```cpp
using namespace tbs::sql;

MysqlConnection conn;
conn.connect("Database=mydb;User=user;Password=pass;Server=127.0.0.1;Port=3306");

MysqlCommand cmd(&conn);
cmd.prepare("INSERT INTO sample (id, name) VALUES (?, ?)");

MysqlParameterCollection params;
params.push_back(std::make_shared<MysqlParameter>("id", DataType::int32, 10));
params.push_back(std::make_shared<MysqlParameter>("name", DataType::string, std::string("alice")));

cmd.bind(params);
cmd.execute();

params[0]->value(11);
params[1]->value(std::string("bob"));
cmd.bind(params);
cmd.execute();

cmd.reset();
cmd.close();
```

This is the actual prepared-statement flow at the driver level. The driver may create a native statement object internally, but the library gives you a consistent cross-driver API instead of forcing you to work with each backend's raw statement system directly.

## Simple summary

- `SqlQuery` is the easy app-level API.
- Driver command classes are the real backend implementation.
- Parameters are supported and safe to use.
- Named placeholders are rewritten automatically.
- The library supports prepared execution without building SQL strings by hand.

In plain English: TobasaSQL lets you write SQL with parameters, and the library handles the backend details for you.