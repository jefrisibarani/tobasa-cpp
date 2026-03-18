#include <iomanip>
#include "tobasa/util_date.h"

namespace tbs {
namespace util {

bool parseDate( tbsdate::sys_seconds& time, const std::string& dateStr, const std::string& format)
{
   std::istringstream ss { dateStr };
   ss >> tbsdate::parse(format, time);

   if (ss.fail())
      return false;
   else
      return true;
}

bool parseDate(std::tm& time, const std::string& dateStr, const std::string& format)
{
   std::istringstream ss(dateStr);
   //ss.imbue(std::locale(""));
   ss >> std::get_time(&time, format.c_str());

   if (ss.fail())
      return false;
   else
      return true;
}

std::string formatDate( tbsdate::sys_seconds& time, std::string_view format)
{
   std::string result = tbsfmt::vformat(format, tbsfmt::make_format_args(time));
   return result;
}

std::string formatDateNow(std::string_view format)
{
#if (defined(_MSC_VER) && _MSC_VER >= 1930)  // Visual Studio 2022 version 17.0.1 Up
   // Convert system_clock::now() to sys_seconds for formatting
   auto now = std::chrono::time_point_cast<tbsdate::sys_seconds::duration>(std::chrono::system_clock::now());
   std::string result = tbsfmt::vformat(format, tbsfmt::make_format_args(now));
   return result;
#else
   // std::chrono::system_clock::now() returns a std::chrono::time_point, 
   // which isn't directly formattable by the std::format facilities in C++20.
   auto currentTime = floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
   std::string result = util::formatDate(format, currentTime);
   return result;
#endif
}

/** @}*/

} // namespace str
} // namespace tbs