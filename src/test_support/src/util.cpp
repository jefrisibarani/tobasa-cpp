#include <tobasa/datetime.h>
#include "tobasa_test_support/util.h"

namespace tbs {
namespace test_support {
   
std::string normalizeTime(const std::string& timeStr)
{
   tbs::DateTime dt;
   if (!dt.parseTime(timeStr, "%H:%M:%S"))
      return "";

   auto valTime = dt.format("{:%H:%M:%S}");
   return valTime;
}

std::string normalizeDate(const std::string& dateStr)
{
   tbs::DateTime dt;
   if (!dt.parse(dateStr, "%Y-%m-%d"))
      return "";

   auto valTime = dt.format("{:%Y-%m-%d}");
   return valTime;
}

std::string normalizeDateTime(const std::string& dateTimeStr)
{
   tbs::DateTime dt;
   if (!dt.parse(dateTimeStr, "%Y-%m-%d %H:%M:%S"))
      return "";

   auto valTime = dt.format("{:%Y-%m-%d %H:%M:%S}");
   return valTime;
}


}} // tbs::test_support