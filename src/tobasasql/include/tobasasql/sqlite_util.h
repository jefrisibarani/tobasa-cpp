#pragma once

#include "tobasasql/common_types.h"
#include "tobasasql/sqlite_type.h"

namespace tbs {
namespace sql {

/** \addtogroup SQL
   @{
 */

 /// Convert SqliteType enum from std::string.
SqliteType sqliteTypeFromString(const std::string& type);

/// Convert SqliteType enum to std::string.
std::string sqliteTypeToString(SqliteType type);

/// Convert sql::DataType to SqliteType.
SqliteType sqliteTypeFromDataType(DataType type);

/// Convert SqliteType to sql::DataType.
DataType sqliteTypeToDataType(SqliteType type);

/// Get SQLite column declared type
std::string sqliteColumnDeclaredType(sqlite3_stmt* stmt, int pos);

DataType sqliteDeclaredTypeToDataType(sqlite3_stmt* stmt, int pos);

/// Get correct Sqlite Data type from declared column data type.
SqliteType sqliteTypeFromDeclaredType(const std::string& colType);

/// Get Type class from SQLite type.
TypeClass typeClassFromSqliteDeclaredType(const std::string& colType);



/** @}*/

} // namespace sql
} // namespace tbs