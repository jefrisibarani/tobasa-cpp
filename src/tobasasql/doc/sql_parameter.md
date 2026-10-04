# SQL Parameter

A SQL parameter has a name, data type, and value.

Example:

```cpp
MysqlParameter("id", DataType::integer, 10);
```

The parameter name is `id`.

## When the name is not used for binding

For MySQL, PostgreSQL, and SQLite, parameters are usually bound by position.

```sql
SELECT * FROM users WHERE id = ? AND name = ?
```

The first parameter is bound to the first `?`.
The second parameter is bound to the second `?`.

In this case, parameter order is important. The names are mostly metadata.

## Strict Parameter Order

TobasaSQL uses strict parameter order.

Parameters are stored in a list and sent to the database in that same order:

```text
parameter[0] -> first placeholder
parameter[1] -> second placeholder
parameter[2] -> third placeholder
```

Example:

```cpp
query.addParam("id", DataType::integer, 10);
query.addParam("name", DataType::varchar, "Alice");
```

This must match:

```sql
WHERE id = ? AND name = ?
```

The parameter name does not automatically find the correct placeholder. The
parameter collection must be built in SQL placeholder order.

This is different from an indexed binding API:

```cpp
statement.bind(2, "Alice");
statement.bind(1, 10);
```

With an indexed API, values can be bound in any order because the index tells
the binder where each value belongs. The driver can later send the complete
binding array in index order.


## When the name is important

The name is important in some parts of the library.

### SqlTable

`SqlTable` uses the parameter name as a column name.

```cpp
table.addParameter("patient_id", DataType::integer, 10);
```

This can produce:

```sql
WHERE patient_id = :param1
```

The name must match a real table column.

### ADO

ADO uses the parameter name when creating an ADO parameter.

Because of this, the name should be correct when using `AdodbConnection`.

### SqlParameterRewriter

`SqlParameterRewriter` uses names when SQL contains named placeholders.

Example:

```sql
SELECT * FROM users WHERE id = :id AND name = :name
```

The rewriter finds `id` and `name`, keeps their order, and changes the SQL to
the placeholder style required by the driver.

For MySQL, the result is:

```sql
SELECT * FROM users WHERE id = ? AND name = ?
```

For PostgreSQL, the result is:

```sql
SELECT * FROM users WHERE id = $1 AND name = $2
```

The rewriter does not bind values by itself. It only rewrites the SQL and
returns the parameter names in order. The driver then binds values by position.

### ODBC logging

ODBC mainly binds parameters by position. However, the name is used in logs and
error messages.

## Summary

| Flow | Importance of parameter name |
| --- | --- |
| `MysqlCommand` | Mostly metadata; binding uses parameter order |
| PostgreSQL | Mostly metadata; binding uses parameter order |
| SQLite | Mostly metadata; binding uses parameter order |
| ODBC | Metadata, logging, and diagnostics; binding uses position |
| ADO | Important for the native ADO parameter name |
| `SqlTable` | Important because it is used as a column name |
| `SqlParameterRewriter` | Important when using named placeholders; finds names and keeps their order |

## Practical rule

Use clear and correct parameter names.

For normal positional queries:

- parameter order controls binding;
- parameter names help readability and logging.

For `SqlTable` and ADO:

- parameter names can affect the query or driver behavior;
- do not use empty or incorrect names.

The name is not only a label, but its importance depends on the database driver
and API being used.
