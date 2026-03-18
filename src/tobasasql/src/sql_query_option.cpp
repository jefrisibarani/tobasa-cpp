#include "tobasasql/sql_query_option.h"

namespace tbs {
namespace sql {

bool SqlQueryOption::limitOffsetValid() const
{
   return (limit >=0 && offset >= 0 );
}

bool SqlQueryOption::dateRangeValid() const
{
   bool valid = (!startDate.empty() && !endDate.empty());
   if (valid)
   {
      //startdate = startdate + " 00:00:00";
      //endDate   = endDate + " 00:00:00";
   }

   return valid;
}

} // namespace sql
} // namespace tbs
