#pragma once

#include "tobasa/date.h"

// https://github.com/HowardHinnant/date/issues/565

namespace tbs {

/** 
 * @ingroup TBS
 * DateTime class - Local time.
 */

using LocalTimeMilis = tbsdate::local_time<std::chrono::milliseconds>;
using ZonedTimeMilis = tbsdate::zoned_time<std::chrono::milliseconds>;
using SysTimeMilis   = tbsdate::sys_time<std::chrono::milliseconds>;


/**
 * @brief The DateTime class provides functionality for handling date and time.
 * 
 * This class supports various operations such as initialization from Unix time,
 * parsing date-time strings, formatting date-time objects, and retrieving ISO 8601
 * formatted strings. It also includes methods to check for null DateTime objects
 * and to set them to null.
 */
class DateTime
{
public:

   /**
    * @brief Initializes the DateTime time zone.
    * This static method must be called before creating any DateTime object.
    * 
    * @param dataFolder The folder path containing the timezone database.
    * When Tobasa is compiled with TOBASA_USE_STD_DATE, std::chrono will handle the timezone database loading,
    * so there is no need to provide the timezone database.
    * When not compiled with TOBASA_USE_STD_DATE, DateTime will use HowardHinnant's date library.
    */
   static bool initTimezoneData(const std::string& dataFolder="");
   static bool usingInMemoryTZDB();

   DateTime();

  /**
    * @brief Constructs a DateTime object from a Unix time in milliseconds.
    * 
    * This constructor creates a DateTime object from a provided std::chrono::system_clock::time_point,
    * which is initialized with the Unix time in milliseconds.
    * 
    * Example:
    * @code
    *   long long utcMs = 1685409828477;
    *    std::chrono::system_clock::time_point tp{std::chrono::milliseconds{utcMs}};
    *    
    *    // C++ 20
    *    // std::chrono::sys_time tp{std::chrono::milliseconds{utcMs}};
    * 
    *    DateTime dt(tp);
    * @endcode
    * @param timePoint A time_point representing the Unix time in milliseconds.
    */
   DateTime(const std::chrono::system_clock::time_point& timePoint);
   
   /**
    * @brief Constructs a DateTime object from a local time point.
    * 
    * This constructor creates a DateTime object from a provided LocalTimeMilis object,
    * representing a local time point.
    * 
    * Example:
    * @code
    *    auto expiredTime = DateTime();
    *    DateTime expiredTime2(expiredTime.timePoint());
    * @endcode 
    * @param timePoint A LocalTimeMilis object representing the local time point.
    */  
   DateTime(const LocalTimeMilis& timePoint);

   ~DateTime();

   /**
    * @brief Get the current DateTime representing the current system time.
    */
   static DateTime now();

   /**
    * @brief Parses a date-time string into a DateTime object.
    * Parses a provided date-time string into a DateTime object using the specified format.
    * If no format is provided, the default format is "%Y-%m-%d %H:%M:%S".
    * 
    * @param dateStr The date-time string to be parsed.
    * @param format The format of the date-time string.
    * @return True if parsing is successful, false otherwise.
    * @note when parsing failed, the DateTime object will be set to epoch (0 milliseconds since epoch)
    * use isNullDateTime() to check if the DateTime object is null
    * 
    * Note: failed when parsing time with this syntax: dt.parse("05:23:42", "%H:%M:%S")
    * To parse time correctly we need add a dummy date 
    *   eg: dt.parse("1970-01-01 05:23:42", "%Y-%m-%d %H:%M:%S")
    **/
   bool parse(const std::string& dateStr, const std::string& format = "%Y-%m-%d %H:%M:%S");

   /**
    * @brief Parses a time string into a DateTime object.
    * Parses a provided time string into a DateTime object using the specified format.
    * If no format is provided, the default format is "%H:%M:%S".
    * 
    * @param timeStr The date-time string to be parsed.
    * @param format The format of the date-time string.
    * @return True if parsing is successful, false otherwise.
    * @note when parsing failed, the DateTime object will be set to epoch (0 milliseconds since epoch)
    * use isNullDateTime() to check if the DateTime object is null
    * 
    * Note: failed when parsing time with this syntax: dt.parse("05:23:42", "%H:%M:%S")
    * To parse time correctly we need add a dummy date 
    *   eg: dt.parse("1970-01-01 05:23:42", "%Y-%m-%d %H:%M:%S")
    **/
   bool parseTime(const std::string& timeStr, const std::string& format = "%H:%M:%S");

   /**
    * @brief Formats the DateTime object into a string.
    * 
    * Formats the DateTime object into a string using the provided format string.
    * The default format string is "{:%Y-%m-%d}". Optionally, includes the second fraction
    * and supports UTC formatting.
    * 
    * @param formatStr The format specifier for the DateTime string.
    * @param useSecondFraction Flag to include the second fraction in the formatted string.
    * @param utc Flag to indicate UTC time formatting.
    * @return A string containing the formatted DateTime object.
    * @note If the DateTime object is null, the string "null" is returned.
    */
   std::string format(std::string_view formatStr = "{:%Y-%m-%d}", bool useSecondFraction = false, bool utc=false);

   /**
    * @brief Retrieves the ISO 8601 formatted date.
    * @return An ISO 8601 formatted date string.
    * @note If the DateTime object is null, the string "null" is returned.
    */
   std::string isoDateString();

   /**
    * @brief Retrieves the ISO 8601 formatted time string.
    * @return An ISO 8601 formatted time string.
    * @note If the DateTime object is null, the string "null" is returned.
    */
   std::string isoTimeString(bool useSecondFraction = false);

   /**
    * @brief Retrieves the ISO 8601 formatted date and time string.
    * @return An ISO 8601 formatted date time string.
    * @note If the DateTime object is null, the string "null" is returned.
    */
   std::string isoDateTimeString(bool useSecondFraction = false);
   
   /**
    * @brief Retrieves the ISO 8601 formatted date and time string in UTC.
    * @return An ISO 8601 formatted date time string.
    * @note If the DateTime object is null, the string "null" is returned.
    */   
   std::string isoDateTimeStringUTC(bool useSecondFraction = false);

   /**
    * @brief Get internal time point.
    *
    * Example:
    * @code
    *  auto expiredTime = DateTime();
    *  expiredTime.timePoint() +=  std::chrono::minutes{60}; 
    *
    *  DateTime expiredTime2(expiredTime.timePoint());
    *  bool notOk = expiredTime2.timePoint() > expiredTime.timePoint();
    *  expiredTime2.timePoint() += std::chrono::years{2};  
    *  long long interval  = ( expiredTime2.timePoint() - expiredTime.timePoint() ).count();
    *  @endcode
    */
   LocalTimeMilis& timePoint();

   tbsdate::year_month_day ymd();

   tbsdate::hh_mm_ss<std::chrono::milliseconds> hms();

   long long toUnixTimeMiliSeconds();

   long long toUnixTimeSeconds();

   /**
    * @brief Checks if the DateTime object is null.
    * @return True if the DateTime object is null, false otherwise.
    */
   bool isNullDateTime();

   /// @brief Sets the DateTime object to null.
   void setToNullDateTime();

private:

   void init(std::chrono::system_clock::time_point);
   ZonedTimeMilis zonedTime();
   std::string    _dateInputStr;
   LocalTimeMilis _timepoint;
   
   inline static bool _timeZoneInitialized = false;
   inline static bool _usingInMemoryTZDB = false;

   bool _setToNull = false;
};

} // namespace tbs