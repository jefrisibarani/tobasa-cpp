#include <iostream>
#include "tobasa/path.h"
#include "tobasa/datetime.h"
#include "tobasa/util_date.h"

namespace tbs {

bool DateTime::initTimezoneData(const std::string& dataFolder)
{
#if defined(TOBASA_USE_STD_DATE)

   try
   {
      auto zone = tbsdate::current_zone();
   }
   catch(const std::exception& ex)
   {
      std::cerr << "DateTime error: " <<  ex.what() << "\n";
      return false;
   }
   std::cout << "Using stdlib time zone" << "\n";
   DateTime::_timeZoneInitialized = true;
   return true;

#elif defined(TOBASA_DATE_USE_OS_TZDB)
   #if WIN32
     # error TOBASA_DATE_USE_OS_TZDB not available on Windows!
   #endif

   try
   {
      auto zone = tbsdate::current_zone();
   }
   catch(const std::exception& ex)
   {
      std::cerr << "DateTime error: " <<  ex.what() << "\n";
      return false;
   }
   std::cout << "Using OS time zone DB" << "\n";
   DateTime::_timeZoneInitialized = true;
   return true;

#else

   try
   {
      // Default tzdata directory
      std::string tzDataFolder = path::executableDir() + path::SEPARATOR + "tzdata";

      if (!dataFolder.empty())
         tzDataFolder = dataFolder;

      std::cout << "Initializing time zone DB\n";

      if (path::exists(tzDataFolder))
      {
         std::cout << "Using time zone DB from: " << tzDataFolder << "\n";
         tbsdate::set_install(tzDataFolder);
      }
      else 
      {
    #ifdef TOBASA_DATE_USE_IN_MEMORY_TZDB
         std::cout << "Time zone DB folder:" << tzDataFolder <<  " does not exist" << "\n";
         std::cout << "Using in-memory time zone DB" << "\n";
         _usingInMemoryTZDB = true;
    #else
         std::cerr << "DateTime error: No valid source available for time zone DB" << "\n";
    #endif
      }

      // force date library to load timezone database
      auto zone = tbsdate::current_zone();
   }
   catch(const std::exception& ex)
   {
      std::cerr << "DateTime error: " <<  ex.what() << "\n";
      return false;
   }

   DateTime::_timeZoneInitialized = true;
   return true;

#endif
}

bool DateTime::usingInMemoryTZDB() 
{
   return _usingInMemoryTZDB;
}


DateTime::DateTime()
{
   if( !_timeZoneInitialized)
      throw std::runtime_error("Time zone not initialized");

   // get System Time miliseconds
   auto now = floor<std::chrono::milliseconds>(std::chrono::system_clock::now());

   try
   {
      // convet to Local Time miliseconds
      _timepoint = tbsdate::current_zone()->to_local(now);
   }
   catch(const std::exception& ex)
   {
      std::string msg = std::string("DateTime error: ") + ex.what();
      throw std::runtime_error(msg);
   }

   /*
#ifdef TOBASA_USE_STD_DATE
   ZonedTimeMilis _zonedTime1 = { tbsdate::current_zone(), now };
   ZonedTimeMilis _zonedTime2 = tbsdate::zoned_time{ tbsdate::current_zone(), _timepoint };
#else
   ZonedTimeMilis _zonedTime1 = tbsdate::make_zoned(tbsdate::current_zone(), now);
   ZonedTimeMilis _zonedTime2 = tbsdate::make_zoned(tbsdate::current_zone(), _timepoint);
#endif
   */
   /*
   _timepoint	= 1674604397282 : (GMT)Tuesday, January 24, 2023 11:53:17.282 PM
                                 (WIB)Wednesday, January 25, 2023 6:53:17.282 AM GMT+07:00

   _now        = 1674579197282   (GMT)Tuesday, January 24, 2023 4:53:17.282 PM
                                 (WIB)Tuesday, January 24, 2023 11:53:17.282 PM GMT+07:00
   _zonedTime1	= 1674579197282
   _zonedTime2	= 1674579197282
   */
}

DateTime::DateTime(const std::chrono::system_clock::time_point& timePoint)
{
   init(timePoint);
}

DateTime::DateTime(const LocalTimeMilis& timePoint)
{
   _timepoint = timePoint;
}

DateTime::~DateTime()
{}

void DateTime::init(std::chrono::system_clock::time_point timePoint)
{
   auto tp = floor<std::chrono::milliseconds>(timePoint);
   auto tpLocal = tbsdate::current_zone()->to_local(tp);
   _timepoint = tpLocal;
}

DateTime DateTime::now()
{
   return DateTime();
}

bool DateTime::parse(const std::string& dateStr, const std::string& format)
{
   LocalTimeMilis tp;
   std::istringstream ss{ dateStr };
   //ss.imbue(std::locale::classic());
   ss >> tbsdate::parse(format, tp);

   if (ss.fail()) 
   {
      setToNullDateTime();

      return false;
   }
   else
   {
      _setToNull = false;
      _timepoint = tp;
      return true;
   }
}

bool DateTime::parseTime(const std::string& timeStr, const std::string& format)
{
   std::string dateTimeStr = "1000-01-01 " + timeStr;
   std::string fmtStr = "%Y-%m-%d " + format;
   return parse(dateTimeStr, fmtStr);
}

std::string DateTime::format(std::string_view formatStr, bool useSecondFraction, bool utc)
{
   if (isNullDateTime()) {
      return "null";
   }

   if (utc)
   {
      if (useSecondFraction)
         return util::formatDate(formatStr, zonedTime().get_sys_time());
      else
      {
         auto tp = floor<std::chrono::seconds>(zonedTime().get_sys_time());
         return util::formatDate(formatStr, tp);
      }
   }
   else
   {
      if (useSecondFraction)
         return util::formatDate(formatStr, _timepoint);
      else
      {
         auto tp = floor<std::chrono::seconds>(_timepoint);
         return util::formatDate(formatStr, tp);
      }
   }
}

std::string DateTime::isoDateString()
{
   if (isNullDateTime()) {
      return "null";
   }

   // ISO 8601
   return util::formatDate("{:%Y-%m-%d}", _timepoint);
}

std::string DateTime::isoTimeString(bool useSecondFraction)
{
   if (isNullDateTime()) {
      return "null";
   }

   // ISO 8601
   if (useSecondFraction)
      return util::formatDate("{:%H:%M:%S}", _timepoint);
   else
   {
      auto tp = floor<std::chrono::seconds>(_timepoint);
      return util::formatDate("{:%H:%M:%S}", tp);
   }
}

std::string DateTime::isoDateTimeString(bool useSecondFraction)
{
   if (isNullDateTime()) {
      return "null";
   }

   // ISO 8601
   if (useSecondFraction)
   {
      // same result
      //return util::formatDate("{:%Y-%m-%d %H:%M:%S}", zonedTime());
      //return util::formatDate("{:%Y-%m-%d %H:%M:%S}", zonedTime().get_local_time());
      return util::formatDate("{:%Y-%m-%d %H:%M:%S}", _timepoint);
   }
   else
   {
      // same result
      //auto tp = floor<std::chrono::seconds>(zonedTime().get_local_time());
      auto tp = floor<std::chrono::seconds>(_timepoint);
      return util::formatDate("{:%Y-%m-%d %H:%M:%S}", tp);
   }
}

std::string DateTime::isoDateTimeStringUTC(bool useSecondFraction)
{
   if (isNullDateTime()) {
      return "null";
   }

   if (useSecondFraction)
      return util::formatDate("{:%Y-%m-%d %H:%M:%S}", zonedTime().get_sys_time());
   else
   {
      auto tp = floor<std::chrono::seconds>(zonedTime().get_sys_time());
      return util::formatDate("{:%Y-%m-%d %H:%M:%S}", tp);
   }
}

LocalTimeMilis& DateTime::timePoint()
{
   return _timepoint;
}

tbsdate::year_month_day DateTime::ymd()
{
   auto timePointDays = floor<tbsdate::days>(_timepoint);
   return tbsdate::year_month_day{ timePointDays };
}

tbsdate::hh_mm_ss<std::chrono::milliseconds> DateTime::hms()
{
   auto timePointDays = floor<tbsdate::days>(_timepoint);
   return tbsdate::hh_mm_ss{ _timepoint - timePointDays };
}


long long DateTime::toUnixTimeMiliSeconds()
{
#ifdef TOBASA_USE_STD_DATE
   tbsdate::zoned_time zt{ tbsdate::current_zone(), _timepoint };
#else
   tbsdate::zoned_time zt = tbsdate::make_zoned(tbsdate::current_zone(), _timepoint);
#endif
   //auto st = zt.get_sys_time();
   //auto ut = st.time_since_epoch();
   //return ut.count();

   return zt.get_sys_time().time_since_epoch().count();
}

long long DateTime::toUnixTimeSeconds()
{
#ifdef TOBASA_USE_STD_DATE
   tbsdate::zoned_time zt{ tbsdate::current_zone(), _timepoint };
#else
   tbsdate::zoned_time zt = tbsdate::make_zoned(tbsdate::current_zone(), _timepoint);
#endif
   auto st = zt.get_sys_time();
   auto s = std::chrono::duration_cast<std::chrono::seconds >( st.time_since_epoch() );
   return s.count();
}

ZonedTimeMilis DateTime::zonedTime()
{
#ifdef TOBASA_USE_STD_DATE
   return tbsdate::zoned_time{ tbsdate::current_zone(), _timepoint };
#else
   return tbsdate::make_zoned(tbsdate::current_zone(), _timepoint);
#endif
}

bool DateTime::isNullDateTime()
{
   return ( _setToNull == true );
} 

void DateTime::setToNullDateTime() 
{
   _setToNull = true;
   _timepoint = LocalTimeMilis{};
}

} // namespace tbs