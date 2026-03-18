#include <tobasa/format.h>
#include "tobasasql/odbc_helper.h"

namespace tbs {
namespace sql {

std::string OdbcHelper::limitAndOffset(long limit, long offset, const std::string& dbmsName)
{
   if(dbmsName == "Microsoft SQL Server")
      return tbsfmt::format(" OFFSET {} ROWS FETCH NEXT {} ROWS ONLY", offset, limit);
   else
      return tbsfmt::format(" LIMIT {} OFFSET {}", limit, offset );
}

} // namespace sql
} // namespace tbs