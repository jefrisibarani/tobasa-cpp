#pragma once

#include <string>
#include "tobasa/date.h"
#include "tobasa/format.h"

namespace tbs {
namespace util {

/** @addtogroup TBS
 * @{
 */

#if defined(TOBASA_USE_STD_DATE) && defined(TOBASA_USE_STD_FORMAT)

   /**
    * @brief Formats a date with a specified format
    * @note The 'formatStr' parameter uses the std::format formatting syntax.
    * @code
    *   Example:
    *   auto currentTime = floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
    *   auto currentTimeStr = util::formatDate("{:%Y-%m-%d %H:%M:%S}", currentTime);
    * @endcode
    * 
    * @tparam Args Variadic template for additional arguments.
    * @param[in]  formatStr  Format specifier for the output date string using std::format syntax.
    * @param[in]  args       Additional arguments to be formatted into the string.
    * @return     String containing the formatted date according to the specified format.
    */
   template <typename... Args>
   std::string formatDate(std::string_view formatStr, Args&&... args)
   {
      return tbsfmt::vformat(formatStr, tbsfmt::make_format_args(args...));
   }

#else

   /**
    * @brief Formats a date with a specified format
    * @note The 'formatStr' parameter uses the std::format formatting syntax.
    * @code
    *   Example:
    *   auto currentTime = floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
    *   auto currentTimeStr = util::formatDate("{:%Y-%m-%d %H:%M:%S}", currentTime);
    * @code
    * 
    * \tparam Streamable Type that represents a date/time object or streamable entity.
    * @param[in]  formatStr  Format specifier for the output date string using std::format syntax.
    * @param[in]  tp         The date/time object or streamable entity to be formatted.
    * @return     String containing the formatted date according to the specified format.
    */
   template <class Streamable>
   std::string formatDate(std::string_view formatStr, const Streamable& tp)
   {
      // {:%Y-%m-%d}
      if (formatStr.length() > 0 )
      {
         try
         {
            if (formatStr.at(0) == '{')
               formatStr.remove_prefix(1);

            if (formatStr.at(0) == ':')
               formatStr.remove_prefix(1);

            auto trimPos = formatStr.find('}');

            if (trimPos != formatStr.npos)
               formatStr.remove_suffix(formatStr.size() - trimPos);

            std::string fmt{formatStr};
            
            return date::format(fmt, tp);
         }
         catch(const std::exception& /*ex*/)
         {
            throw std::runtime_error("Invalid date format string parameter");
         }
      }

      throw std::runtime_error("Invalid date format string parameter");
   }
#endif

/**
 * @brief Parses a date string into a tbsdate::sys_seconds.
 * @param[out] time    Reference to a tbsdate::sys_seconds object to store the parsed date and time.
 * @param[in]  dateStr Date/time string to be parsed.
 * @param[in]  format  Format of the input date/time string.
 * @return     Returns true if parsing is successful; otherwise, returns false.
 */
bool parseDate(tbsdate::sys_seconds& time, const std::string& dateStr, const std::string& format);

/**
 * @brief Parses a date string into a std::tm object.
 * @param[out] time    Reference to a std::tm object to store the parsed date and time.
 * @param[in]  dateStr Date/time string to be parsed.
 * @param[in]  format  Format of the input date/time string.
 * @return     Returns true if parsing is successful; otherwise, returns false.
 */
bool parseDate(std::tm& time, const std::string& dateStr, const std::string& format);

/**
 * @brief Formats a date using tbsfmt::format.
 * @param[in]  time   tbsdate::sys_seconds object representing the date to be formatted.
 * @param[in]  format Format specifier for the output date string.
 * @return     String containing the formatted date according to the given format.
 */
std::string formatDate(tbsdate::sys_seconds& time, std::string_view format);

/**
 * @brief Formats the current UTC date and time using tbsfmt::format.
 * @param[in]  format Format specifier for the output date/time string.
 * @return     String containing the formatted current date and time in UTC.
 */
std::string formatDateNow(std::string_view format);

/** @}*/

} // namespace str
} // namespace tbs