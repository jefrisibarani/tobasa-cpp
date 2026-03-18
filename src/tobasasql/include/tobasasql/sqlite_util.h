#pragma once

#include "tobasasql/common_types.h"
#include "tobasasql/sqlite_type.h"

namespace tbs {
namespace sql {

/** \addtogroup SQL
   @{
 */

 /// Convert SqliteType enum from std::string.
SqliteType sqliteTypeFromString(const std::string& ftype);

/// Convert SqliteType enum to std::string.
std::string sqliteTypeToString(SqliteType type);

/// Convert sql::DataType to ADODB::DataTypeEnum.
SqliteType sqliteTypeFromDataType(DataType type);

/// Convert SqliteType Enum to sql::DataType.
DataType sqliteTypeToDataType(SqliteType ftype);

/** 
 * \brief Parse SQLite declared column data type into "standard" data type.
 * \details 
 * Parse data type returned from sqlite3_column_decltype()
 * then transform to our "standard" types:
 * 
 * Integer, Float, Double, Real, Numeric,
 * Char, VarChar, Text
 * Blob, Bool,
 * Date, DateTime, TimeStamp
 */
std::string sqliteColumnDeclaredType(std::string ftype);


/** 
 * \brief Get correct Sqlite Data type from declared column data type.
 * \details Input parameter colType should be consistent with
 * output from sqliteColumnDeclaredType() :
 * 
 * Integer, Float, Double, Real, Numeric,
 * Char, VarChar, Text
 * Blob, Bool,
 * Date, DateTime, TimeStamp
 */
SqliteType sqliteTypeFromDeclaredType(const std::string& ftype);


/** 
 * \brief Get Type class from SQLite type.
 * \details Input parameter colType should be consistent with
 * output from sqliteColumnDeclaredType() :
 * 
 * Integer, Float, Double, Real, Numeric,
 * Char, VarChar, Text
 * Blob, Bool,
 * Date, DateTime, TimeStamp
 */
TypeClass typeClassFromSqliteDeclaredType(std::string colType);

/** @}*/

} // namespace sql
} // namespace tbs