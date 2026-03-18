#pragma once

#include <string>
#include <any>

namespace tbs {
namespace sql {

/**
 * @brief SQL database query options
 * Store limit and offset 
 * Store start date and end date
 * Store optional filter if any 
 */
struct SqlQueryOption
{
   SqlQueryOption() {}
   SqlQueryOption(int queryLimit, int queryOffset, const std::string& filterCode="")
      : limit(queryLimit)
      , offset(queryOffset)
      , filter(filterCode)
   {}

   SqlQueryOption(int optLimit, int optOffset, 
                  const std::string& optStartDate, const std::string& optEndDate, const std::string& optFilter="")
      : limit(optLimit)
      , offset(optOffset)
      , startDate(optStartDate)
      , endDate(optEndDate)
      , filter(optFilter)
   {}

   int limit  = -1;
   int offset = -1;
   std::string startDate;
   std::string endDate;
   std::string filter;
   bool hasInvalidParameter = false;
   std::any data;

   bool limitOffsetValid() const;

   bool dateRangeValid() const;
};


} // namespace sql
} // namespace tbs