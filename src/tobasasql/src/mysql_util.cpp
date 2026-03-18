#include "tobasasql/mysql_util.h"

/*

+ -----------------+ ----------+ ---------------------+ ---------------------+ ------------------------------------------------------- +
|   MySQL Type     |  Internal |       VariantType    | C++ Type             |   Notes                                                 |
+ -----------------+ ----------+ ---------------------+ ---------------------+ ------------------------------------------------------- +
| TINYINT          | tinyint   | int8_t / uint8_t     | int8_t / uint8_t     | Signed or unsigned (check is_unsigned flag).            |
| SMALLINT         | smallint  | int16_t / uint16_t   | int16_t / uint16_t   | Usually fits in 2 bytes.                                |
| MEDIUMINT        | integer   | int32_t              | int32_t              | MySQL-specific 3-byte integer, mapped to 32-bit signed. |
| INT, INTEGER     | integer   | int32_t / uint32_t   | int32_t / uint32_t   | Common 4-byte integer.                                  |
| BIGINT           | bigint    | int64_t / uint64_t   | int64_t / uint64_t   | Large 8-byte integer.                                   |
| BIT              | tinyint   | int8_t / uint8_t     | int8_t / uint8_t     | Treated as boolean or bit field.                        |
| BOOL, BOOLEAN    | boolean   | bool                 | bool                 | Alias for TINYINT(1).                                   |
| DECIMAL, NUMERIC | numeric   | std::string          | std::string          | Exact fixed-point numeric.                              |
| FLOAT            | float4    | float                | float                | 4-byte single precision.                                |
| DOUBLE, REAL     | float8    | double               | double               | 8-byte double precision.                                |
| DATE             | date      | std::string          | std::string          | YYYY-MM-DD date only.                                   |
| DATETIME         | timestamp | std::string          | std::string          | Full timestamp without timezone.                        |
| TIMESTAMP        | timestamp | std::string          | std::string          | UTC-based timestamp, may auto-update.                   |
| TIME             | time      | std::string          | std::string          | Time of day (HH:MM:SS).                                 |
| YEAR             | integer   | int32_t              | int32_t              | 2 or 4-digit year, MySQL-specific.                      |
| CHAR(n)          | varchar   | std::string          | std::string          | Fixed-length string (padded with spaces).               |
| VARCHAR(n)       | varchar   | std::string          | std::string          | Variable-length string.                                 |
|                  |           |                      |                      |                                                         |
| TINYTEXT         | text      | std::vector<char>    | std::string          | Character large object (CLOB).                          |
| TEXT             | text      | std::vector<char>    | std::string          | Character large object (CLOB).                          |
| MEDIUMTEXT       | text      | std::vector<char>    | std::string          | Character large object (CLOB).                          |
| LONGTEXT         | text      | std::vector<char>    | std::string          | Character large object (CLOB).                          |
|                  |           |                      |                      |                                                         |
| BINARY(n)        | varbinary | std::vector<uint8_t> | std::vector<uint8_t> | Fixed-length binary.                                    |
| VARBINARY(n)     | varbinary | std::vector<uint8_t> | std::vector<uint8_t> | Variable-length binary.                                 |
|                  |           |                      |                      |                                                         |
| TINYBLOB         | varbinary | std::vector<uint8_t> | std::vector<uint8_t> | Binary large object (BLOB).                             |
| BLOB             | varbinary | std::vector<uint8_t> | std::vector<uint8_t> | Binary large object (BLOB).                             |
| MEDIUMBLOB       | varbinary | std::vector<uint8_t> | std::vector<uint8_t> | Binary large object (BLOB).                             |
| LONGBLOB         | varbinary | std::vector<uint8_t> | std::vector<uint8_t> | Binary large object (BLOB).                             |
|                  |           |                      |                      |                                                         |
| JSON             | text      | std::string          | std::string          | Stored as text JSON representation.                     |
|                  |           |                      |                      |                                                         | 
| ENUM             | varchar   | std::string          | std::string          | Stored as string or numeric index.                      |
| SET              | varchar   | std::string          | std::string          | Comma-separated multiple enum values.                   |
| GEOMETRY         | varbinary | std::vector<uint8_t> | std::vector<uint8_t> | MySQL spatial (WKB format).                             |
| POINT            | varbinary | std::vector<uint8_t> | std::vector<uint8_t> | MySQL spatial (WKB format).                             |
| LINESTRING       | varbinary | std::vector<uint8_t> | std::vector<uint8_t> | MySQL spatial (WKB format).                             |
| NULL             | Null      | Null                 | std::monostate       | Represents SQL NULL value.                              |
+ -----------------+ ----------| ---------------------| ---------------------+ ------------------------------------------------------- +

*/


namespace tbs {
namespace sql {

MySqlType mysqlDataTypeFromString(const std::string& ftype)
{
   if (ftype == "decimal")
      return MySqlType::MYSQL_TYPE_DECIMAL;
   else if (ftype == "tiny")
      return MySqlType::MYSQL_TYPE_TINY;
   else if (ftype == "short")
      return MySqlType::MYSQL_TYPE_SHORT;
   else if (ftype == "long")
      return MySqlType::MYSQL_TYPE_LONG;
   else if (ftype == "float")
      return MySqlType::MYSQL_TYPE_FLOAT;
   else if (ftype == "double")
      return MySqlType::MYSQL_TYPE_DOUBLE;
   else if (ftype == "null")
      return MySqlType::MYSQL_TYPE_NULL;
   else if (ftype == "timestamp")
      return MySqlType::MYSQL_TYPE_TIMESTAMP;
   else if (ftype == "longlong")
      return MySqlType::MYSQL_TYPE_LONGLONG;
   else if (ftype == "int24")
      return MySqlType::MYSQL_TYPE_INT24;
   else if (ftype == "date")
      return MySqlType::MYSQL_TYPE_DATE;
   else if (ftype == "time")
      return MySqlType::MYSQL_TYPE_TIME;
   else if (ftype == "datetime")
      return MySqlType::MYSQL_TYPE_DATETIME;
   else if (ftype == "year")
      return MySqlType::MYSQL_TYPE_YEAR;
   else if (ftype == "newdate")
      return MySqlType::MYSQL_TYPE_NEWDATE;
   else if (ftype == "varchar")
      return MySqlType::MYSQL_TYPE_VARCHAR;
   else if (ftype == "bit")
      return MySqlType::MYSQL_TYPE_BIT;
   else if (ftype == "timestamp2")
      return MySqlType::MYSQL_TYPE_TIMESTAMP2;
   else if (ftype == "datetime2")
      return MySqlType::MYSQL_TYPE_DATETIME2;
   else if (ftype == "time2")
      return MySqlType::MYSQL_TYPE_TIME2;
   else if (ftype == "json")
      return MySqlType::MYSQL_TYPE_JSON;
   else if (ftype == "newdecmal")
      return MySqlType::MYSQL_TYPE_NEWDECIMAL;
   else if (ftype == "enum")
      return MySqlType::MYSQL_TYPE_ENUM;
   else if (ftype == "tiny_blob")
      return MySqlType::MYSQL_TYPE_TINY_BLOB;
   else if (ftype == "medium_blob")
      return MySqlType::MYSQL_TYPE_MEDIUM_BLOB;
   else if (ftype == "long_blob")
      return MySqlType::MYSQL_TYPE_LONG_BLOB;
   else if (ftype == "blob")
      return MySqlType::MYSQL_TYPE_BLOB;
   else if (ftype == "var_string")
      return MySqlType::MYSQL_TYPE_VAR_STRING;
   else if (ftype == "string")
      return MySqlType::MYSQL_TYPE_STRING;
   else if (ftype == "string")
      return MySqlType::MYSQL_TYPE_GEOMETRY;
   else
      throw TypeException("Invalid string conversion to MySQL data type", "MysqlUtil");
}

std::string mysqlDataTypeToString(MySqlType ftype)
{
   std::string retVal;

   switch (ftype)
   {
   case MySqlType::MYSQL_TYPE_DECIMAL:
      return "decimal";
   case MySqlType::MYSQL_TYPE_TINY:
      return "tiny";
   case MySqlType::MYSQL_TYPE_SHORT:
      return "short";
   case MySqlType::MYSQL_TYPE_LONG:
      return "long";
   case MySqlType::MYSQL_TYPE_FLOAT:
      return "float";
   case MySqlType::MYSQL_TYPE_DOUBLE:
      return "double";
   case MySqlType::MYSQL_TYPE_NULL:
      return "null";
   case MySqlType::MYSQL_TYPE_TIMESTAMP:
      return "timestamp";
   case MySqlType::MYSQL_TYPE_LONGLONG:
      return "longlong";
   case MySqlType::MYSQL_TYPE_INT24:
      return "int24";
   case MySqlType::MYSQL_TYPE_DATE:
      return "date";
   case MySqlType::MYSQL_TYPE_TIME:
      return "time";
   case MySqlType::MYSQL_TYPE_DATETIME:
      return "datetime";
   case MySqlType::MYSQL_TYPE_YEAR:
      return "year";
   case MySqlType::MYSQL_TYPE_NEWDATE:
      return "newdate";
   case MySqlType::MYSQL_TYPE_VARCHAR:
      return "varchar";
   case MySqlType::MYSQL_TYPE_BIT:
      return "bit";
   case MySqlType::MYSQL_TYPE_TIMESTAMP2:
      return "timestamp2";
   case MySqlType::MYSQL_TYPE_DATETIME2:
      return "datetime2";
   case MySqlType::MYSQL_TYPE_TIME2:
      return "time2";
   case MySqlType::MYSQL_TYPE_JSON:
      return "json";
   case MySqlType::MYSQL_TYPE_NEWDECIMAL:
      return "newdecimal";
   case MySqlType::MYSQL_TYPE_ENUM:
      return "enum";
   case MySqlType::MYSQL_TYPE_SET:
      return "set";
   case MySqlType::MYSQL_TYPE_TINY_BLOB:
      return "tiny_blob";
   case MySqlType::MYSQL_TYPE_MEDIUM_BLOB:
      return "medium_blob";
   case MySqlType::MYSQL_TYPE_LONG_BLOB:
      return "long_blob";
   case MySqlType::MYSQL_TYPE_BLOB:
      return "blob";
   case MySqlType::MYSQL_TYPE_VAR_STRING:
      return "var_string";
   case MySqlType::MYSQL_TYPE_STRING:
      return "string";
   case MySqlType::MYSQL_TYPE_GEOMETRY:
      return "geometry";
   default:
      throw TypeException("Invalid MySQL data type conversion to string", "MysqlUtil");
   }

   return retVal;
}

MySqlType mysqlDataTypeFromDataType(DataType type)
{
   MySqlType retVal;

   switch (type)
   {
   case DataType::tinyint:
      return MySqlType::MYSQL_TYPE_TINY;
   case DataType::smallint:
      return MySqlType::MYSQL_TYPE_SHORT;
   case DataType::integer:
      return MySqlType::MYSQL_TYPE_LONG;
   case DataType::bigint:
      return MySqlType::MYSQL_TYPE_LONGLONG;
   case DataType::numeric:
      return MySqlType::MYSQL_TYPE_DECIMAL;
   case DataType::float4:
      return MySqlType::MYSQL_TYPE_FLOAT;
   case DataType::float8:
      return MySqlType::MYSQL_TYPE_DOUBLE;
   case DataType::boolean:
      return MySqlType::MYSQL_TYPE_TINY;
   case DataType::character:
   case DataType::varchar:
   case DataType::text:
      return MySqlType::MYSQL_TYPE_STRING; //MySqlType::MYSQL_TYPE_VARCHAR;
   //case DataType::text:
   //   return MySqlType::MYSQL_TYPE_BLOB;
   case DataType::date:
      return MySqlType::MYSQL_TYPE_DATE;
   case DataType::time:
      return MySqlType::MYSQL_TYPE_TIME;
   case DataType::timestamp:
      return MySqlType::MYSQL_TYPE_DATETIME;
   case DataType::varbinary:
      return MySqlType::MYSQL_TYPE_BLOB;
   default:
      throw TypeException("Invalid DataType conversion to MySQL data type", "MysqlUtil");
   }

   return retVal;
}

DataType mysqlDataTypeToDataType(MySqlType ftype)
{
   DataType retVal;

   switch (ftype)
   {
   case MySqlType::MYSQL_TYPE_TINY:
      return DataType::tinyint;
   case MySqlType::MYSQL_TYPE_SHORT:
      return DataType::smallint;
   case MySqlType::MYSQL_TYPE_INT24:
   case MySqlType::MYSQL_TYPE_LONG:
      return DataType::integer;
   case MySqlType::MYSQL_TYPE_LONGLONG:
      return DataType::bigint;
   case MySqlType::MYSQL_TYPE_DECIMAL:
   case MySqlType::MYSQL_TYPE_NEWDECIMAL:
      return DataType::numeric;
   /* TODO_JEFRI: 
      By default, MySQL treats REAL as a synonym for DOUBLE
      unless the REAL_AS_FLOAT SQL mode is enabled
   */      
   case MySqlType::MYSQL_TYPE_FLOAT:
      return DataType::float4;
   case MySqlType::MYSQL_TYPE_DOUBLE:
      return DataType::float8;
   case MySqlType::MYSQL_TYPE_BIT:
      return DataType::tinyint;
   case MySqlType::MYSQL_TYPE_STRING:
   case MySqlType::MYSQL_TYPE_VAR_STRING:
   case MySqlType::MYSQL_TYPE_VARCHAR:
      return DataType::varchar;
   case MySqlType::MYSQL_TYPE_JSON:
      return DataType::text;
   case MySqlType::MYSQL_TYPE_TINY_BLOB:
   case MySqlType::MYSQL_TYPE_MEDIUM_BLOB:
   case MySqlType::MYSQL_TYPE_LONG_BLOB:
   case MySqlType::MYSQL_TYPE_BLOB:
      return DataType::varbinary;
   case MySqlType::MYSQL_TYPE_YEAR:
      return DataType::integer;
   case MySqlType::MYSQL_TYPE_DATE:
   case MySqlType::MYSQL_TYPE_NEWDATE:
      return DataType::date;
   case MySqlType::MYSQL_TYPE_TIME:
      return DataType::time;
   case MySqlType::MYSQL_TYPE_TIMESTAMP:
   case MySqlType::MYSQL_TYPE_DATETIME:
      return DataType::timestamp;
   default:
      throw TypeException("Invalid MySQL data type conversion to DataType", "MysqlUtil");
   }

   return retVal;
}

DataType mysqlDataTypeToDataType(long ftype)
{
   MySqlType pgType = static_cast<MySqlType>(ftype);

   return mysqlDataTypeToDataType(pgType);
}

TypeClass typeClassFromMySqlType(const long type)
{
   TypeClass retVal;
   MySqlType myType = (MySqlType)type;

   switch (myType)
   {
   case MySqlType::MYSQL_TYPE_NULL:
   case MySqlType::MYSQL_TYPE_TINY:
   case MySqlType::MYSQL_TYPE_SHORT:
   case MySqlType::MYSQL_TYPE_LONG:
   case MySqlType::MYSQL_TYPE_FLOAT:
   case MySqlType::MYSQL_TYPE_DOUBLE:
   case MySqlType::MYSQL_TYPE_LONGLONG:
   case MySqlType::MYSQL_TYPE_INT24:
   case MySqlType::MYSQL_TYPE_DECIMAL:
   case MySqlType::MYSQL_TYPE_NEWDECIMAL:
      return TypeClass::numeric;
   case MySqlType::MYSQL_TYPE_TINY_BLOB:
   case MySqlType::MYSQL_TYPE_MEDIUM_BLOB:
   case MySqlType::MYSQL_TYPE_LONG_BLOB:
   case MySqlType::MYSQL_TYPE_BLOB:
   {
      return TypeClass::blob;
   }
   case MySqlType::MYSQL_TYPE_BIT:
      return TypeClass::boolean;
   
   case MySqlType::MYSQL_TYPE_JSON:
   case MySqlType::MYSQL_TYPE_VARCHAR:
   case MySqlType::MYSQL_TYPE_VAR_STRING:
   case MySqlType::MYSQL_TYPE_STRING:
   {
      return TypeClass::string;
   }
   case MySqlType::MYSQL_TYPE_TIMESTAMP:
   case MySqlType::MYSQL_TYPE_DATE:
   case MySqlType::MYSQL_TYPE_TIME:
   case MySqlType::MYSQL_TYPE_DATETIME:
   case MySqlType::MYSQL_TYPE_YEAR:
   case MySqlType::MYSQL_TYPE_NEWDATE:
      return TypeClass::date;
   default:
      throw TypeException(tbsfmt::format("Invalid MySQL data type conversion to TypeClass: {}", type), "MysqlUtil");
   }

   return retVal;
}

} // namespace sql
} // namespace tbs