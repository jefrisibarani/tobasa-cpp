#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#include "tobasasql/adodb_helper.h"

namespace tbs {
namespace sql {

std::string AdodbHelper::limitAndOffset(long limit, long offset, const std::string& dbmsName)
{
   std::string limitOffset = tbsfmt::format(" OFFSET {} ROWS FETCH NEXT {} ROWS ONLY", offset, limit);
   return limitOffset;
}

} // namespace sql
} // namespace tbs

#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)