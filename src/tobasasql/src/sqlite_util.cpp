#include <string>
#include <tobasa/format.h>
#include <tobasa/exception.h>
#include <tobasa/util_string.h>
#include <tobasasql/sql_util.h>
#include "tobasasql/sqlite_util.h"

namespace tbs {
namespace sql {

SqliteType sqliteTypeFromString(const std::string& type)
{
   if (type == "null")
      return SqliteType::null;
   else if (type == "integer")
      return SqliteType::integer;
   else if (type == "real")
      return SqliteType::real;
   else if (type == "text")
      return SqliteType::text;
   else if (type == "blob")
      return SqliteType::blob;
   else
      throw TypeException("Invalid SQLite data type conversion from string", "SQLiteUtil");
}

std::string sqliteTypeToString(SqliteType type)
{
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
}

SqliteType sqliteTypeFromDataType(DataType type)
{
   switch (type)
   {
   case DataType::tinyint:
   case DataType::smallint:
   case DataType::integer:
   case DataType::bigint:
   case DataType::boolean:
      return SqliteType::integer;
   case DataType::float4:
   case DataType::float8:
      return SqliteType::real;
   case DataType::numeric:
   case DataType::character:
   case DataType::varchar:
   case DataType::text:
   case DataType::date:
   case DataType::time:
   case DataType::timestamp:
      return SqliteType::text;
   case DataType::varbit:
   case DataType::varbinary:
      return SqliteType::blob;
   default:
      throw TypeException("Invalid DataType conversion to SQLite data type", "SQLiteUtil");
   }
}

DataType sqliteTypeToDataType(SqliteType type)
{
   switch (type)
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
}

std::string sqliteColumnDeclaredType(sqlite3_stmt* stmt, int pos)
{
   // Note: https://www.sqlite.org/c3ref/column_decltype.html
   // The first parameter is a prepared statement. If this statement is a SELECT statement 
   // and the Nth column of the returned result set of that SELECT is a table column 
   // (not an expression or subquery) then the declared type of the table column is returned. 
   // If the Nth column of the result set is an expression or subquery, 
   // then a NULL pointer is returned. The returned string is always UTF-8 encoded.

   const char* colTypStr = sqlite3_column_decltype(stmt, pos);
   if (!colTypStr) {
      return "DECLTYPE_UNKNOWN";
   }

   return colTypStr;
}

DataType sqliteDeclaredTypeToDataType(sqlite3_stmt* stmt, int pos)
{
   auto declaredType = sqliteColumnDeclaredType(stmt,pos);
   return dataTypeFromDeclaredType(declaredType);
}

SqliteType sqliteTypeFromDeclaredType(const std::string& type)
{
   DataType dataType = dataTypeFromDeclaredType(type);
   return sqliteTypeFromDataType(dataType);
}

TypeClass typeClassFromSqliteDeclaredType(const std::string& type)
{
   DataType dataType = dataTypeFromDeclaredType(type);
   return typeClassFromDataType(dataType);
}


} // namespace sql
} // namespace tbs