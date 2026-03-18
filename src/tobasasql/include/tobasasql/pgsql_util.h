#pragma once

#include <string>
#include <tobasasql/sql_parameter.h>
#include "tobasasql/pgsql_type.h"

namespace tbs {
namespace sql {

/** \addtogroup SQL
   @{
 */

 /// Convert PostgreSQL data type from std::string.
PgsqlType pgsqlDataTypeFromString(const std::string& ftype);

/// Convert PostgreSQL data type to string.
std::string pgsqlDataTypeToString(PgsqlType ftype);

/// Convert sql::DataType to PostgreSQL data type.
PgsqlType pgsqlDataTypeFromDataType(DataType type);

/// Convert PostgreSQL data type to sql::DataType.
DataType pgsqlDataTypeToDataType(PgsqlType ftype);

/// Convert PostgreSQL data type to sql::DataType.
DataType pgsqlDataTypeToDataType(long ftype);

/// Get Type class from PostgreSQL type.
TypeClass typeClassFromPgsqlType(const long type);

/** @}*/

} // namespace sql
} // namespace tbs