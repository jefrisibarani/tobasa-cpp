# TobasaSQL data types

This page documents the portable SQL type model used by TobasaSQL and the
backend-specific mappings implemented in the driver utility code under
`include/tobasasql/*_util.h/.cpp`.

Use `tbs::sql::DataType` when binding parameters or interpreting result-column
metadata. It is a portable abstraction for the common SQL types used by the
library; it is not intended to preserve every backend-specific native type.

## Portable data types

| `DataType` | Meaning | Portable C++ value representation |
| --- | --- | --- |
| `tinyint` | 8-bit unsigned integer; documented range 0..255 | `uint8_t` |
| `smallint` | Signed 16-bit integer | `int16_t` |
| `integer` | Signed 32-bit integer | `int32_t` |
| `bigint` | Signed 64-bit integer | `int64_t` |
| `numeric` | Exact, arbitrary-precision decimal | `std::string` containing the decimal text |
| `float4` | 32-bit floating point | `float` |
| `float8` | 64-bit floating point | `double` |
| `boolean` | Logical true/false | `bool` |
| `character` | Fixed-length character string | `std::string` |
| `varchar` | Variable-length character string | `std::string` |
| `text` | Large/unbounded character data | `std::string` |
| `date` | Date without a time | `std::string`, normally `YYYY-MM-DD` |
| `time` | Time without a time zone | `std::string`, normally `HH:MM:SS` |
| `timestamp` | Date and time without a time zone | `std::string`, normally `YYYY-MM-DD HH:MM:SS` |
| `varbinary` | Binary/blob data | `std::vector<uint8_t>` |
| `varbit` | Bit-string / bit-like binary data. e.g `10101010` | `std::vector<uint8_t>` |
| `unknown` | No portable mapping | No defined portable representation |

SQL `NULL` is not a `DataType` member. A result value can nevertheless be
stored as `std::monostate` by the backend variant.

## Mapping conventions

Each table is read in two directions:

* **TobasaSQL -> backend**: the native type selected by the
  `*DataTypeFromDataType` helper when binding a parameter or creating a
  portable schema mapping.
* **Backend -> TobasaSQL**: the portable type produced by the
  `*DataTypeToDataType` helper when reading metadata from a result column.


## SQLite

SQLite result values use SQLite runtime storage classes, not declared column
types: `NULL`, `INTEGER`, `REAL`, `TEXT`, and `BLOB`.

| TobasaSQL type | SQLite type when sent | SQLite type -> TobasaSQL | Notes |
| --- | --- | --- | --- |
| `tinyint` | `INTEGER` | `INTEGER` -> `bigint` | SQLite integer results are read with `sqlite3_column_int64`. |
| `smallint` | `INTEGER` |  | Same runtime class as every SQLite integer. |
| `integer` | `INTEGER` |  |  |
| `bigint` | `INTEGER` |  |  |
| `numeric` | `REAL` | `REAL` -> `float8` | Exact decimal semantics are not preserved by this mapping. |
| `float4` | `REAL` |  |  |
| `float8` | `REAL` |  |  |
| `boolean` | `INTEGER` |  | SQLite has no boolean storage class. |
| `character` | `TEXT` | `TEXT` -> `varchar` |  |
| `varchar` | `TEXT` |  |  |
| `text` | `TEXT` |  |  |
| `date` | `TEXT` |  | SQLite date values are stored as text. |
| `time` | `TEXT` |  | SQLite time values are stored as text. |
| `timestamp` | `TEXT` |  | SQLite timestamp values are stored as text. |
| `varbinary` | `BLOB` | `BLOB` -> `varbinary` | Result BLOB bytes are converted to a hexadecimal `std::string` in `SqliteResult`; this differs from the documented portable `vector<uint8_t>` representation. |
| `varbit` | `BLOB` | `BLOB` -> `varbinary` | SQLite has no native `BIT`/`VARBIT` type. The implementation treats bit-like values as binary blobs and normalizes them as `varbinary` on read. |
| `unknown` | -- |  | Throws when converting to SQLite. |

The reverse mapping is intentionally lossy: SQLite `INTEGER` is normalized to
`bigint`, `REAL` to `float8`, and `TEXT` to `varchar`, regardless of the
original declared type. SQLite `NULL` is stored as `std::monostate` in a value
variant, while `sqliteTypeToDataType(SqliteType::null)` still reports `varchar`
for metadata purposes.

SQLite declared-type inspection uses a compatibility parser for common type
names such as `INTEGER`, `BOOLEAN`, `CHAR`, `VARCHAR`, `TEXT`, `DOUBLE`,
`FLOAT`, `REAL`, `NUMERIC`, `BLOB`, `DATE`, `DATETIME`, and `TIMESTAMP`.
Unrecognized declared types are treated as text by `sqliteColumnDeclaredType`.

## PostgreSQL

The PostgreSQL native type is identified by the OID represented by
`tbs::sql::PgsqlType`.

| PostgreSQL native type | TobasaSQL -> PostgreSQL | PostgreSQL -> TobasaSQL | Notes |
| --- | --- | --- | --- |
| `int2` (OID 21) | `smallint`, `tinyint` | `smallint` | `tinyint` is widened to `int2`. |
| `int4` (OID 23) | `integer` | `integer` |  |
| `int8` (OID 20) | `bigint` | `bigint` |  |
| `serial` | -- | -- | `serial` is declared in `PgsqlType` but is not handled by either data-type conversion switch. |
| `serial8` / `bigserial` | -- | -- | `serial8` is declared, but not handled by either data-type conversion switch. |
| `boolean` (OID 16) | `boolean` | `boolean` |  |
| `numeric` / `decimal` (OID 1700) | `numeric` | `numeric` |  |
| `float4` / `real` (OID 700) | `float4` | `float4` |  |
| `float8` / `double precision` (OID 701) | `float8` | `float8` |  |
| `money` (OID 790) | -- | `float8` | Reverse conversion treats money as `float8`. |
| `character` (OID 18) | -- | `character` | This is PostgreSQL's internal one-byte type, not SQL `CHAR(1)`. |
| `bpchar` / `char(n)` (OID 1042) | `character` -> `varchar` | `varchar` |  |
| `varchar` (OID 1043) | `character`, `varchar` -> `varchar` | `varchar` |  |
| `text` (OID 25) | `text` | `text` |  |
| `name` (OID 19) | -- | `varchar` | PostgreSQL internal identifier type. |
| `xml` (OID 142) | -- | `varchar` | Returned as text. |
| `bit` (OID 1560) | `varbit` | `varbit` | PostgreSQL `BIT(n)` is mapped as a bit-string/binary value, not as text. |
| `varbit` (OID 1562) | `varbit` | `varbit` | Variable-length bit string. |
| `bytea` (OID 17) | `varbinary` | `varbinary` |  |
| `date` (OID 1082) | `date` | `date` |  |
| `time` (OID 1083) | `time` | `time` |  |
| `timetz` (OID 1266) | -- | `time` | Time-zone information is not represented separately. |
| `timestamp` (OID 1114) | `timestamp` | `timestamp` |  |
| `timestamptz` (OID 1184) | -- | `timestamp` | Time-zone information is not represented separately. |
| `interval` (OID 1186) | -- | -- | Classified as a date-like type, but has no `DataType` conversion case. |
| `char_array`, `bpchar_array`, `varchar_array` | -- | `varchar` | Returned as text if encountered. |
| `oid`, `tid`, `xid`, `cid` | -- | `bigint` | PostgreSQL identifier/transaction types are widened to `int64_t`. |
| `json`, `jsonb`, UUID, arrays, ranges, network types, enums, `tsvector`, `tsquery` | -- | -- | No conversion case in `pgsqlDataTypeToDataType`. |

## MySQL / MariaDB

The MySQL driver uses `enum_field_types` (`MySqlType`). The reverse mapping
also consults `BINARY_FLAG`: string/blob fields without that flag are exposed
as `varchar`, even when their native enum is a string or blob type.

| MySQL native type | TobasaSQL -> MySQL | MySQL -> TobasaSQL | Notes |
| --- | --- | --- | --- |
| `MYSQL_TYPE_TINY` (`TINYINT`) | `tinyint`, `boolean` | `tinyint` | Boolean parameters use the same native type. Signedness is not retained in `DataType`. |
| `MYSQL_TYPE_SHORT` (`SMALLINT`) | `smallint` | `smallint` |  |
| `MYSQL_TYPE_INT24` (`MEDIUMINT`) | -- | `integer` |  |
| `MYSQL_TYPE_LONG` (`INT`, `INTEGER`) | `integer` | `integer` |  |
| `MYSQL_TYPE_LONGLONG` (`BIGINT`) | `bigint` | `bigint` |  |
| `MYSQL_TYPE_DECIMAL`, `MYSQL_TYPE_NEWDECIMAL` | `numeric` | `numeric` |  |
| `MYSQL_TYPE_FLOAT` (`FLOAT`) | `float4` | `float4` |  |
| `MYSQL_TYPE_DOUBLE` (`DOUBLE`, usually `REAL`) | `float8` | `float8` |  |
| `MYSQL_TYPE_BIT` (`BIT`) | `varbit` | `varbit` | The portable representation remains `std::vector<uint8_t>`. The MySQL result layer converts raw bytes to a display string such as `10101010`, but the value is not a `varchar`. |
| `MYSQL_TYPE_STRING`, `MYSQL_TYPE_VAR_STRING`, `MYSQL_TYPE_VARCHAR` | `character`, `varchar`, `text` -> `STRING` | `varchar` for non-binary fields | Parameter text uses `MYSQL_TYPE_STRING`, not `MYSQL_TYPE_VARCHAR`. |
| `MYSQL_TYPE_JSON` (`JSON`) | -- | `text` | JSON is returned as text. |
| `MYSQL_TYPE_TINY_BLOB`, `MEDIUM_BLOB`, `LONG_BLOB`, `BLOB` | `varbinary` -> `BLOB` | `varbinary` |  |
| `MYSQL_TYPE_DATE`, `MYSQL_TYPE_NEWDATE` | `date` -> `DATE` | `date` |  |
| `MYSQL_TYPE_TIME` | `time` -> `TIME` | `time` |  |
| `MYSQL_TYPE_TIMESTAMP`, `MYSQL_TYPE_DATETIME` | `timestamp` -> `DATETIME` | `timestamp` |  |
| `MYSQL_TYPE_YEAR` (`YEAR`) | -- | `integer` |  |
| `MYSQL_TYPE_ENUM`, `MYSQL_TYPE_SET` | -- | -- | Native-to-string formatting exists for enum/set, but the data-type reverse switch has no case. |
| `MYSQL_TYPE_GEOMETRY` | -- | -- | Formatting exists, but no data-type conversion case. |
| `MYSQL_TYPE_NULL` | -- | -- | Formatting exists, but reverse data-type conversion throws. SQL NULL values are handled separately by result variants. |


## ODBC

ODBC mappings use the standard `SQL_*` type constants. The ODBC driver adjusts
`SQL_FLOAT` using reported precision: precision 1..24 remains `SQL_FLOAT`, and
precision 25..53 is treated as `SQL_DOUBLE` for column metadata.

| ODBC type | TobasaSQL -> ODBC | ODBC -> TobasaSQL | Notes |
| --- | --- | --- | --- |
| `SQL_TINYINT` | `tinyint` | `tinyint` |  |
| `SQL_SMALLINT` | `smallint` | `smallint` |  |
| `SQL_INTEGER` | `integer` | `integer` |  |
| `SQL_BIGINT` | `bigint` | `bigint` |  |
| `SQL_DECIMAL`, `SQL_NUMERIC` | `numeric` | `numeric` |  |
| `SQL_REAL` | `float4` | `float4` |  |
| `SQL_FLOAT` | -- | `float4` or `float8` | Parameter `float4` uses `SQL_REAL`; result precision can promote `SQL_FLOAT` to `SQL_DOUBLE`. |
| `SQL_DOUBLE` | `float8` | `float8` |  |
| `SQL_BIT` | `boolean` | `boolean` |  |
| `SQL_CHAR` | `character` | `character` |  |
| `SQL_VARCHAR`, `SQL_WCHAR`, `SQL_WVARCHAR` | `varchar`, `text` -> `SQL_WVARCHAR` | `varchar` |  |
| `SQL_LONGVARCHAR`, `SQL_WLONGVARCHAR` | -- | `text` |  |
| `SQL_BINARY`, `SQL_VARBINARY`, `SQL_LONGVARBINARY` | `varbinary` -> `SQL_VARBINARY` | `varbinary` |  |
| `SQL_TYPE_DATE` | `date` | `date` |  |
| `SQL_TYPE_TIME`, driver-specific `-154` (`SQL_SS_TIME2`) | `time` | `time` |  |
| `SQL_TYPE_TIMESTAMP`, driver-specific `-155` (`SQL_SS_TIMESTAMPOFFSET`) | `timestamp` | `timestamp` | Offset information is not represented separately. |

Other ODBC native types, including interval and GUID types, have no conversion
case in `odbcTypeToDataType` and cause `TypeException` when metadata is read.

## ADO / SQL Server

The ADO backend is compiled only when `TOBASA_SQL_USE_ADODB` and `_MSC_VER` are
defined. It uses `ADODB::DataTypeEnum` and COM variants.

| ADO type | TobasaSQL -> ADO | ADO -> TobasaSQL | Notes |
| --- | --- | --- | --- |
| `adTinyInt` | `tinyint` | `tinyint` | SQL Server `tinyint` is unsigned at the database level; the portable model does not preserve that fact. |
| `adSmallInt` | `smallint` | `smallint` |  |
| `adInteger` | `integer` | `integer` |  |
| `adBigInt` | `bigint` | `bigint` |  |
| `adDecimal`, `adNumeric` | `numeric` | `numeric` |  |
| `adSingle` | `float4` | `float4` |  |
| `adDouble` | `float8` | `float8` |  |
| `adBoolean` | `boolean` | `boolean` | COM uses `VT_BOOL`. |
| `adChar` | `character` | `character` |  |
| `adVarWChar` | `varchar` | `varchar` |  |
| `adWChar` | `character` | `varchar` | Reverse mapping distinguishes only `adChar` as fixed character. |
| `adLongVarWChar`, `adLongVarChar` | `text` | `text` |  |
| `adDBDate`, `adDate` | `date` -> `adDBDate` | `date` | `adDate` is also accepted as date on read. |
| `adDBTime` | `time` | `time` |  |
| `adDBTimeStamp` | `timestamp` | `timestamp` |  |
| `adDBTimeX` / enum value 145 | -- | `timestamp` | The source maps the SQL Server time compatibility value 145 to timestamp. |
| `adVarBinary`, `adBinary`, `adLongVarBinary` | `varbinary` -> `adVarBinary` | `varbinary` |  |
| `adCurrency` | -- | `unknown` | No portable money type. |
| `adUnsignedTinyInt`, `adUnsignedSmallInt`, `adUnsignedInt`, `adUnsignedBigInt` | -- | `unknown` | Unsigned ADO types are not reverse-mapped to portable integer types. |
| `adGUID`, `adVariant`, `adError`, `adUserDefined`, `adArray`, and other ADO-only types | -- | `unknown` | The conversion helper returns `DataType::unknown` for these cases. |

ADO parameter values use the backend-specific `ComVariantType`, which includes
COM `_variant_t` in addition to the common scalar, string, and binary
alternatives. The portable type mapping does not mean that all native ADO
values can be represented by `DataType`.

## Source references

The mappings documented here are implemented in the following files:

* `include/tobasasql/common_types.h` — `DataType` and its string names.
* `include/tobasasql/sqlite_type.h` and `src/sqlite_util.cpp`.
* `include/tobasasql/pgsql_type.h` and `src/pgsql_util.cpp`.
* `include/tobasasql/mysql_util.h` and `src/mysql_util.cpp`.
* `include/tobasasql/odbc_util.h` and `src/odbc_util.cpp`.
* `include/tobasasql/adodb_util.h` and `src/adodb_util.cpp`.
* `src/*_result.cpp` — result value materialization and column metadata.

Use these files when a backend-specific detail needs to be checked against the
actual implementation.
