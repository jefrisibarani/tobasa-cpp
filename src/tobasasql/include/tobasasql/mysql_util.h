#pragma once

#include "tobasasql/exp_imp.h"

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
TBS_SQL_API MySqlType mysqlDataTypeFromString(const std::string& ftype);

/// Convert MySQL data type to string.
TBS_SQL_API std::string mysqlDataTypeToString(MySqlType ftype);

/// Convert sql::DataType to MySQL data type.
TBS_SQL_API MySqlType mysqlDataTypeFromDataType(DataType type);

/// Convert MySQL data type to sql::DataType.
TBS_SQL_API DataType mysqlDataTypeToDataType(MySqlType ftype);

/// Convert MySQL data type to sql::DataType.
TBS_SQL_API DataType mysqlDataTypeToDataType(long ftype);

/// Get Type class from MySQL type.
TBS_SQL_API TypeClass typeClassFromMySqlType(const long type);

/** @}*/

} // namespace sql
} // namespace tbs