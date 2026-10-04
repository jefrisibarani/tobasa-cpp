# Prepared Statement Note

This library supports parameterized SQL, but it is not a full prepared-statement implementation in the usual database sense.

What this means in practice:

- You can pass values through query parameters instead of building raw SQL strings.
- Named parameters like :id are rewritten to the database-specific style before execution.
- The SQL is executed as a normal command, and the driver may create a statement object internally for that single call.

It is not the same as creating a reusable server-side prepared statement object and reusing it many times.

In other words, the library may use driver statement APIs internally, but only for one-shot execution. The public abstraction is still a query-plus-parameter list, not a long-lived prepared statement handle.

So for library users:

- Parameterized queries are supported.
- Reusing the same prepared statement object across multiple executions is not the main abstraction here.
- This is still safer than concatenating SQL strings directly, but it is not the same as a true database prepared statement.

In short: parameter binding is available, but this is not a full prepared-statement system.

Example of the real MySQL lifecycle:

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

This is a true prepared statement at the MySQL driver level. The public `SqlQuery` API still stays simple and one-shot for normal use, while `MysqlCommand` can be used when a reusable statement is needed.
