#include "tobasasql/pgsql_util.h"

/*

+ ----------------------------+ ---+------------------------+ ---------------------+ ------------------------+----------------------------------------------+
| PostgreSQL Type             |    | Internal DataType      | Internal VariantType | C++ Type                | Notes                                        |
+ ----------------------------+ ---+------------------------+ ---------------------+ ------------------------+----------------------------------------------+
| boolean                      [Y] | DataType::boolean      | bool                 | bool                    | Stored as 't'/'f'                            |
| smallint (int2)              [Y] | DataType::smallint     | int16_t              | int16_t                 | 2-byte signed integer                        |
| integer (int4)               [Y] | DataType::integer      | int32_t              | int32_t                 | 4-byte signed integer                        |
| bigint (int8)                [Y] | DataType::bigint       | int64_t              | int64_t                 | 8-byte signed integer                        |
| serial                       [Y] | DataType::integer      | int32_t              | int32_t                 | Auto-increment alias for int4                |
| bigserial                    [Y] | DataType::bigint       | int64_t              | int64_t                 | Auto-increment alias for int8                |
| real (float4)                [Y] | DataType::float4       | float                | float                   | Single-precision float (4 bytes)             |
| double precision (float8)    [Y] | DataType::float8       | double               | double                  | Double-precision float (8 bytes)             |
| numeric(p,s) / decimal(p,s)  [Y] | DataType::numeric      | std::string          | std::string             | Arbitrary-precision decimal, store as string |
| money                        [Y] | DataType::float8       | int64_t              | int64_t                 | Stored as 64-bit integer                     |

| text                         [Y] | DataType::text         | std::string          | std::string             | UTF-8 character types                        | TEXT
| bpchar_array (1014)          [Y] | DataType::varchar      | std::string          | std::string             |                                              | CHAR[]
| varchar_array (1015          [Y] | DataType::varchar      | std::string          | std::string             |                                              | VARCHAR[]
| varbit (1562)                [Y] | DataType::varchar      | std::string          | std::string             |                                              | VARBIT

| bpchar (1042)                [Y] | DataType::varchar      | std::string          | std::string             |                                              | CHARACTER/CHAR,CHARACTER(n)/CHAR(n)
| varchar (1043)               [Y] | DataType::varchar      | std::string          | std::string             |                                              |
| xml (142)                    [Y] | DataType::varchar      | std::string          | std::string             | XML text data                                |
| bit (1560)                   [Y] | DataType::character    | std::string          | std::string             | Fixed-length bit string                      | BIT(n)

| bytea                        [Y] | DataType::varbinary    | std::vector<uint8_t> | std::vector<uint8_t>    | Binary (BLOB) data                           |

| date                         [Y] | DataType::date         | std::string          | same                    | Date only (no time)                          |
| time                         [Y] | DataType::time         | std::string          | same                    | Time of day (no TZ)                          |
| time with time zone          [Y] | DataType::time         | std::string          | same                    | Time + zone offset                           |
| timestamp                    [Y] | DataType::timestamp    | std::string          | same                    | Naive local timestamp                        |
| timestamptz                  [Y] | DataType::timestamp    | std::string          | same                    | Always stored as UTC                         |
| interval (1186)              [N] | DataType::interval     | std::string          |                         | Represents time span                         |

| uuid                         [N] | DataType::varchar      | std::string          | std::string             | 128-bit UUID                                 |
| json / jsonb (114)           [N] | DataType::varchar      | std::string          | std::string             | JSON text format                             |
| jsonpath (4072)              [N] | DataType::varchar      | std::string          | std::string             | JSON path expression                         |

| character (18)               [N] | DataType::tinyint      | std::string          | int8_t                  | PostgreSQL’s 1-byte internal type — not the SQL CHAR(1)
| char_array (1002)            [Y] | DataType::varbinary    | std::vector<int8_t>  | std::vector<int8_t>     | Array of  character (18)                     |
| name (19)                    [Y] | DataType::varchar      | std::string          | std::string             | Internal PostgreSQL identifier               |
| oid                          [Y] | DataType::bigint       | int64_t              | int64_t                 | Object ID                                    |
| xid                          [Y] | DataType::bigint       | int64_t              | int64_t                 | Transaction ID                               |
| cid                          [Y] | DataType::bigint       | int64_t              | int64_t                 | Command ID                                   |
| tid (27)                     [Y] | DataType::bigint       | int64_t              | int64_t                 | Tuple ID (block, offset)                     |
| inet / cidr                  [N] |                        |                      |                         | IP/network address                           |
| macaddr                      [N] |                        |                      |                         | MAC address                                  |
| enum                         [N] |                        |                      |                         | Enum label as text                           |
| array (e.g. int[])           [N] |                        |                      |                         | Array of base elements                       |
| range                        [N] |                        |                      |                         | Inclusive/exclusive range                    |
| tsvector(3614)/tsquery(3615) [N] |                        |                      |                         | Full-text search data                        |
+ ----------------------------+ -----------------------+ ---------------------+ ------------------------+----------------------------------------------+

*/

namespace tbs {
namespace sql {

PgsqlType pgsqlDataTypeFromString(const std::string& ftype)
{
   if (ftype == "boolean")
      return PgsqlType::boolean;
   else if (ftype == "bytea")
      return PgsqlType::bytea;
   else if (ftype == "character")
      return PgsqlType::character;
   else if (ftype == "name")
      return PgsqlType::name;
   else if (ftype == "int8")
      return PgsqlType::int8;
   else if (ftype == "int2")
      return PgsqlType::int2;
   else if (ftype == "int4")
      return PgsqlType::int4;
   else if (ftype == "text")
      return PgsqlType::text;
   else if (ftype == "oid")
      return PgsqlType::oid;
   else if (ftype == "tid")
      return PgsqlType::tid;
   else if (ftype == "xid")
      return PgsqlType::xid;
   else if (ftype == "cid")
      return PgsqlType::cid;
   else if (ftype == "float4")
      return PgsqlType::float4;
   else if (ftype == "float8")
      return PgsqlType::float8;
   else if (ftype == "money")
      return PgsqlType::money;
   else if (ftype == "char_array")
      return PgsqlType::char_array;
   else if (ftype == "bpchar_array")
      return PgsqlType::bpchar_array;
   else if (ftype == "varchar_array")
      return PgsqlType::varchar_array;
   else if (ftype == "bpchar")
      return PgsqlType::bpchar;
   else if (ftype == "varchar")
      return PgsqlType::varchar;
   else if (ftype == "date")
      return PgsqlType::date;
   else if (ftype == "time")
      return PgsqlType::time;
   else if (ftype == "timestamp")
      return PgsqlType::timestamp;
   else if (ftype == "timestamptz")
      return PgsqlType::timestamptz;
   else if (ftype == "interval")
      return PgsqlType::interval;
   else if (ftype == "timetz")
      return PgsqlType::timetz;
   else if (ftype == "bit")
      return PgsqlType::bit;
   else if (ftype == "varbit")
      return PgsqlType::varbit;
   else if (ftype == "numeric")
      return PgsqlType::numeric;
   else if (ftype == "xml")
      return PgsqlType::xml;
   else
      throw TypeException("Invalid string conversion to PostgreSQL data type", "PgsqlUtil");
}

std::string pgsqlDataTypeToString(PgsqlType ftype)
{
   std::string retVal;

   switch (ftype)
   {
   case PgsqlType::boolean:
      return "boolean";
   case PgsqlType::bytea:
      return "bytea";
   case PgsqlType::character:
      return "character";
   case PgsqlType::name:
      return "name";
   case PgsqlType::int8:
      return "int8";
   case PgsqlType::int2:
      return "int2";
   case PgsqlType::int4:
      return "int4";
   case PgsqlType::text:
      return "text";
   case PgsqlType::oid:
      return "oid";
   case PgsqlType::tid:
      return "tid";
   case PgsqlType::xid:
      return "xid";
   case PgsqlType::cid:
      return "cid";
   case PgsqlType::float4:
      return "float4";
   case PgsqlType::float8:
      return "float8";
   case PgsqlType::money:
      return "money";
   case PgsqlType::char_array:
      return "char_array";
   case PgsqlType::bpchar_array:
      return "bpchar_array";
   case PgsqlType::varchar_array:
      return "varchar_array";
   case PgsqlType::bpchar:
      return "bpchar";
   case PgsqlType::varchar:
      return "varchar";
   case PgsqlType::date:
      return "date";
   case PgsqlType::time:
      return "time";
   case PgsqlType::timestamp:
      return "timestamp";
   case PgsqlType::timestamptz:
      return "timestamptz";
   case PgsqlType::interval:
      return "interval";
   case PgsqlType::timetz:
      return "timetz";
   case PgsqlType::bit:
      return "bit";
   case PgsqlType::varbit:
      return "varbit";
   case PgsqlType::numeric:
      return "numeric";
   default:
      throw TypeException("Invalid PostgreSQL data type conversion to string", "PgsqlUtil");
   }

   return retVal;
}

PgsqlType pgsqlDataTypeFromDataType(DataType type)
{
   PgsqlType retVal;

   switch (type)
   {
   case DataType::tinyint:
      return PgsqlType::int2;
   case DataType::smallint:
      return PgsqlType::int2;
   case DataType::integer:
      return PgsqlType::int4;
   case DataType::bigint:
      return PgsqlType::int8;
   case DataType::numeric:
      return PgsqlType::numeric;
   case DataType::float4:
      return PgsqlType::float4;
   case DataType::float8:
      return PgsqlType::float8;
   case DataType::boolean:
      return PgsqlType::boolean;
   case DataType::character:
   case DataType::varchar:
      return PgsqlType::varchar;
   case DataType::text:
      return PgsqlType::text;
   case DataType::date:
      return PgsqlType::date;
   case DataType::time:
      return PgsqlType::time;
   case DataType::timestamp:
      return PgsqlType::timestamp;
   case DataType::varbinary:
      return PgsqlType::bytea;
   default:
      throw TypeException("Invalid DataType conversion to PostgreSQL data type", "PgsqlUtil");
   }

   return retVal;
}

DataType pgsqlDataTypeToDataType(PgsqlType ftype)
{
   DataType retVal;

   switch (ftype)
   {
   case PgsqlType::int2:
      return DataType::smallint;
   case PgsqlType::int4:
      return DataType::integer;
   case PgsqlType::tid:        // TODO_JEFRI:
   case PgsqlType::oid:
   case PgsqlType::xid:
   case PgsqlType::cid:
   case PgsqlType::int8:
      return DataType::bigint;
   case PgsqlType::numeric:
      return DataType::numeric;
   case PgsqlType::float4:
      return DataType::float4;
   case PgsqlType::float8:
   case PgsqlType::money:
      return DataType::float8;
   case PgsqlType::boolean:
      return DataType::boolean;
   case PgsqlType::character:
   case PgsqlType::bit:
      return DataType::character;
   case PgsqlType::name:
   case PgsqlType::char_array:
   case PgsqlType::bpchar_array:
   case PgsqlType::varchar_array:
   case PgsqlType::varbit:
   case PgsqlType::bpchar:
   case PgsqlType::varchar:
   case PgsqlType::xml:
      return DataType::varchar;
   case PgsqlType::text:
      return DataType::text;
   case PgsqlType::bytea:
      return DataType::varbinary;
   case PgsqlType::date:
      return DataType::date;
   case PgsqlType::time:
   case PgsqlType::timetz:
      return DataType::time;
   case PgsqlType::timestamp:
   case PgsqlType::timestamptz:
      return DataType::timestamp;
   
   default:
      throw TypeException("Invalid PostgreSQL data type conversion to DataType", "PgsqlUtil");
   }

   return retVal;
}

DataType pgsqlDataTypeToDataType(long ftype)
{
   PgsqlType pgType = static_cast<PgsqlType>(ftype);

   return pgsqlDataTypeToDataType(pgType);
}

TypeClass typeClassFromPgsqlType(const long type)
{
   TypeClass retVal;
   PgsqlType pgType = (PgsqlType)type;

   switch (pgType)
   {
   case PgsqlType::int2:
   case PgsqlType::int4:
   case PgsqlType::int8:
   case PgsqlType::oid:
   case PgsqlType::xid:
   case PgsqlType::tid:
   case PgsqlType::cid:
   case PgsqlType::float4:
   case PgsqlType::float8:
   case PgsqlType::numeric:
   case PgsqlType::money:
   case PgsqlType::bit:
   case PgsqlType::varbit:
      return TypeClass::numeric;
   case PgsqlType::bytea:
      return TypeClass::blob;
   case PgsqlType::boolean:
      return TypeClass::boolean;
   case PgsqlType::character:
   case PgsqlType::name:
   case PgsqlType::text:
   case PgsqlType::varchar:
   case PgsqlType::char_array:
   case PgsqlType::bpchar_array:
   case PgsqlType::bpchar:
   case PgsqlType::xml:
      return TypeClass::string;
   case PgsqlType::date:
   case PgsqlType::timestamp:
   case PgsqlType::timestamptz:
   case PgsqlType::time:
   case PgsqlType::timetz:
   case PgsqlType::interval:
      return TypeClass::date;

   default:
      throw TypeException(tbsfmt::format("Invalid PostgreSQL data type conversion to TypeClass: {}", type), "PgsqlUtil");
   }

   return retVal;
}

} // namespace sql
} // namespace tbs