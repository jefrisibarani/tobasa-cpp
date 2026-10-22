#pragma once

#include "tobasasql/exp_imp.h"

#include <string>

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 *	\brief Sqlite Helper
 */
struct TBS_SQL_API SqliteHelper
{
   static std::string limitAndOffset(long limit, long offset, const std::string& dbmsName = {});
};


} // namespace sql
} // namespace tbs