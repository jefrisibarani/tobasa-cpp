#include <string>
#include <tobasa/format.h>
#include <tobasa/exception.h>
#include <tobasa/util_string.h>
#include "tobasasql/sqlite_util.h"

namespace tbs {
namespace sql {

SqliteType sqliteTypeFromString(const std::string& ftype)
{
   if (ftype == "null")
      return SqliteType::null;
   else if (ftype == "integer")
      return SqliteType::integer;
   else if (ftype == "real")
      return SqliteType::real;
   else if (ftype == "text")
      return SqliteType::text;
   else if (ftype == "blob")
      return SqliteType::blob;
   else
      throw TypeException("Invalid SQLite data type conversion from string", "SQLiteUtil");
}

std::string sqliteTypeToString(SqliteType type)
{
   std::string retVal;

   switch (type)
   {
   case SqliteType::null:
      return "null";
   case SqliteType::integer:
      return "integer";
   case SqliteType::real:
      return "real";
   case SqliteType::text:
      return "text";
   case SqliteType::blob:
      return "blob";
   default:
      throw TypeException("Invalid SQLite data type conversion to string", "SQLiteUtil");
   }

   return retVal;
}

SqliteType sqliteTypeFromDataType(DataType type)
{
   SqliteType retVal;

   switch (type)
   {
   case DataType::tinyint:
   case DataType::smallint:
   case DataType::integer:
   case DataType::bigint:
   case DataType::boolean:
      return SqliteType::integer;
   case DataType::numeric:
   case DataType::float4:
   case DataType::float8:
      return SqliteType::real;
   case DataType::character:
   case DataType::varchar:
   case DataType::text:
   case DataType::date:
   case DataType::time:
   case DataType::timestamp:
      return SqliteType::text;
   case DataType::varbinary:
      return SqliteType::blob;
   default:
      throw TypeException("Invalid DataType conversion to SQLite data type", "SQLiteUtil");
   }

   return retVal;
}

DataType sqliteTypeToDataType(SqliteType ftype)
{
   DataType retVal;

   switch (ftype)
   {
   case SqliteType::null:
      return DataType::varchar;
   case SqliteType::integer:
      return DataType::bigint;
   case SqliteType::real:
      return DataType::float8;
   case SqliteType::text:
      return DataType::varchar;
   case SqliteType::blob:
      return DataType::varbinary;
   default:
      throw TypeException("Invalid SQLite data type conversion to DataType", "SQLiteUtil");
   }

   return retVal;
}

std::string sqliteColumnDeclaredType(std::string ftype)
{
   std::string columnTypeString = util::toUpper(ftype);

   if (  columnTypeString == "TINYINT"
      || columnTypeString == "SMALLINT"
      || columnTypeString == "MEDIUMINT"
      || columnTypeString == "BIGINT"
      || columnTypeString == "INT"
      || columnTypeString == "INTEGER") 
   {
      return "Integer";
   }
   else if (columnTypeString == "BOOL" || columnTypeString == "BOOLEAN")
      return "Bool";
   else if (util::startsWith(columnTypeString, "CHAR") || util::startsWith(columnTypeString, "CHARACTER"))
      return "Char";
   else if (util::startsWith(columnTypeString, "CHARACTER VARYING")
         || util::startsWith(columnTypeString, "VARCHAR")
         || util::startsWith(columnTypeString, "STRING")) 
   {
      return "VarChar";
   }
   else if (columnTypeString == "TEXT")
      return "Text";
   else if (util::startsWith(columnTypeString, "DOUBLE") || util::startsWith(columnTypeString, "DOUBLE PRECISION"))
      return "Double";
   else if (util::startsWith(columnTypeString, "FLOAT"))
      return "Float";
   else if (columnTypeString == "SINGLE" || columnTypeString == "REAL")
      return "Real";
   else if (util::startsWith(columnTypeString, "NUMERIC") || util::startsWith(columnTypeString, "DECIMAL"))
      return "Numeric";
   else if (columnTypeString == "BLOB")
      return "Blob";
   else if (columnTypeString == "DATE")
      return "Date";
   else if (columnTypeString == "DATETIME")
      return "DateTime";
   else if (columnTypeString == "TIMESTAMP")
      return "TimeStamp";
   else {
      // SQLite very flexible for declared data type
      // For other types, treat as text
      return "Text";
   }
}

SqliteType sqliteTypeFromDeclaredType(const std::string& ftype)
{
   std::string columnTypeString = util::toUpper(ftype);

   if (  columnTypeString == "INTEGER" || columnTypeString == "BOOL")
      return SqliteType::integer;
   else if (columnTypeString == "TEXT"
         || columnTypeString == "CHAR"
         || columnTypeString == "VARCHAR"
         || columnTypeString == "DATE"
         || columnTypeString == "DATETIME"
         || columnTypeString == "TIMESTAMP"
         || columnTypeString == "NUMERIC") 
   {
      return SqliteType::text;
   }
   else if (columnTypeString == "DOUBLE" || columnTypeString == "FLOAT"|| columnTypeString == "REAL")
      return SqliteType::real;
   else if (columnTypeString == "BLOB")
      return SqliteType::blob;
   else if (columnTypeString == "NULL")
      return SqliteType::null;
   else
      throw TypeException(tbsfmt::format("Invalid SQLite data type constructed from declared type: {}", ftype ), "SQLiteUtil");
}

TypeClass typeClassFromSqliteDeclaredType(std::string colType)
{
   if (  colType == "Integer"
      || colType == "Double"
      || colType == "Float"
      || colType == "Real"
      || colType == "Numeric") 
   {
      return TypeClass::numeric;
   }
   else if (colType == "Blob")
      return TypeClass::blob;
   else if (colType == "Bool")
      return TypeClass::boolean;
   else if (colType == "Char" || colType == "VarChar" || colType == "Text")
      return TypeClass::string;
   else if (colType == "Date" || colType == "DateTime" || colType == "TimeStamp")
      return TypeClass::date;
   else
      throw TypeException(tbsfmt::format("Invalid SQLite data type conversion to TypeClass: {}", colType), "SQLiteUtil");
}


} // namespace sql
} // namespace tbs