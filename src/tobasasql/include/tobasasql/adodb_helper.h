#pragma once

#include "tobasasql/exp_imp.h"

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#include <tobasa/format.h>

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 *	\brief Adodb Helper
 */
struct TBS_SQL_API AdodbHelper
{
   static std::string limitAndOffset(long limit, long offset, const std::string& dbmsName = {});
};


} // namespace sql
} // namespace tbs

#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)