#pragma once

#include <string>
#include <tobasasql/sql_parameter.h>
#include <mysql/mysql.h>

namespace tbs {
namespace sql {

/** \addtogroup SQL
   @{
 */

using MySqlType = enum_field_types;


/// Convert MySQL data type from std::string.
MySqlType mysqlDataTypeFromString(const std::string& ftype);

/// Convert MySQL data type to string.
std::string mysqlDataTypeToString(MySqlType ftype);

/// Convert sql::DataType to MySQL data type.
MySqlType mysqlDataTypeFromDataType(DataType type);

/// Convert MySQL data type to sql::DataType.
DataType mysqlDataTypeToDataType(MySqlType ftype);

/// Convert MySQL data type to sql::DataType.
DataType mysqlDataTypeToDataType(long ftype);

/// Get Type class from MySQL type.
TypeClass typeClassFromMySqlType(const long type);

/** @}*/

} // namespace sql
} // namespace tbs