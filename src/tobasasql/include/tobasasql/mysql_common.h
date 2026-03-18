#pragma once

#include <functional>
#include "tobasasql/sql_parameter.h"
#include "tobasasql/mysql_variant.h"

namespace tbs {
namespace sql {

/** \addtogroup SQL
 * @{
 */

/** 
 * MySql Sql Parameter implementation.
 * MysqlParameter accepts MYSQL_TIME as it value
 */
using MysqlParameter               = Parameter<MysqlVariantType>;

/// MysqlParameter shared pointer.
using MysqlParameterPtr            = std::shared_ptr<MysqlParameter>;

/// MysqlParameter collection.
using MysqlParameterCollection     = std::vector<MysqlParameterPtr>;

/// MysqlParameter collection shared pointer.
using MysqlParameterCollectionPtr  = std::shared_ptr<MysqlParameterCollection>;

/** @}*/

} // namespace sql
} // namespace tbs