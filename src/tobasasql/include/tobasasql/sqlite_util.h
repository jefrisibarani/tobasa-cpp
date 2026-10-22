#pragma once

#include "tobasasql/exp_imp.h"

#include "tobasasql/common_types.h"
#include "tobasasql/sqlite_type.h"

namespace tbs {
namespace sql {

/** \addtogroup SQL
   @{
 */

 /// Convert SqliteType enum from std::string.
TBS_SQL_API SqliteType sqliteTypeFromString(const std::string& type);

/// Convert SqliteType enum to std::string.
TBS_SQL_API std::string sqliteTypeToString(SqliteType type);

/// Convert sql::DataType to SqliteType.
TBS_SQL_API SqliteType sqliteTypeFromDataType(DataType type);

/// Convert SqliteType to sql::DataType.
TBS_SQL_API DataType sqliteTypeToDataType(SqliteType type);

/// Get SQLite column declared type
TBS_SQL_API std::string sqliteColumnDeclaredType(sqlite3_stmt* stmt, int pos);

TBS_SQL_API DataType sqliteDeclaredTypeToDataType(sqlite3_stmt* stmt, int pos);

/// Get correct Sqlite Data type from declared column data type.
TBS_SQL_API SqliteType sqliteTypeFromDeclaredType(const std::string& colType);

/// Get Type class from SQLite type.
TBS_SQL_API TypeClass typeClassFromSqliteDeclaredType(const std::string& colType);



/** @}*/

} // namespace sql
} // namespace tbs