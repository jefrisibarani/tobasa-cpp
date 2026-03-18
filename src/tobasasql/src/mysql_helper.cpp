#include <tobasa/format.h>
#include "tobasasql/mysql_helper.h"

namespace tbs {
namespace sql {

std::string MysqlHelper::limitAndOffset(long limit, long offset, const std::string& dbmsName)
{
   std::string limitOffset = tbsfmt::format(" LIMIT {} OFFSET {}", limit, offset);
   return limitOffset;
}


} // namespace sql
} // namespace tbs