#pragma once

#include "tobasasql/exp_imp.h"

#include <string>

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 * \brief Helper
 * \note Tested only with MS SQL Server
 */
struct TBS_SQL_API OdbcHelper
{
   static std::string limitAndOffset(long limit, long offset, const std::string& dbmsName = {});
};


} // namespace sql
} // namespace tbs