#pragma once

#include "tobasasql/exp_imp.h"

#include <string>
#include <tobasasql/sql_parameter.h>
#include "tobasasql/pgsql_type.h"

namespace tbs {
namespace sql {

/** \addtogroup SQL
   @{
 */

 /// Convert PostgreSQL data type from std::string.
TBS_SQL_API PgsqlType pgsqlDataTypeFromString(const std::string& ftype);

/// Convert PostgreSQL data type to string.
TBS_SQL_API std::string pgsqlDataTypeToString(PgsqlType ftype);

/// Convert sql::DataType to PostgreSQL data type.
TBS_SQL_API PgsqlType pgsqlDataTypeFromDataType(DataType type);

/// Convert PostgreSQL data type to sql::DataType.
TBS_SQL_API DataType pgsqlDataTypeToDataType(PgsqlType ftype);

/// Convert PostgreSQL data type to sql::DataType.
TBS_SQL_API DataType pgsqlDataTypeToDataType(long ftype);

/// Get Type class from PostgreSQL type.
TBS_SQL_API TypeClass typeClassFromPgsqlType(const long type);

/** @}*/

} // namespace sql
} // namespace tbs