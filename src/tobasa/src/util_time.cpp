#include <sstream>
#include "tobasa/util_time.h"

namespace tbs {

// Note:
// https://en.cppreference.com/w/c/chrono/localtime
// The function localtime may not be thread-safe.

std::tm TimeUtil::utcTime()
{
   //std::lock_guard< Lock > lock{ _lock };
   std::time_t t = time();
   std::tm res;
#if defined( _MSC_VER )
   gmtime_s( &res, &t );
#elif (defined( __clang__ ) || defined( __GNUC__ )) && !defined(__WIN32__)
   gmtime_r( &t, &res );
#else
   res = *gmtime( &t );
#endif
   return res;
}

std::tm TimeUtil::localTime()
{
   std::lock_guard< std::mutex > lock{ _lock };
   std::time_t t = time();
   std::tm res;

#if defined( _MSC_VER )
   localtime_s( &res, &t );
#elif (defined( __clang__ ) || defined( __GNUC__ )) && !defined(__WIN32__)
   localtime_r( &t, &res );
#else
   res = *localtime( &t );
#endif
   return res;
}

std::time_t TimeUtil::time()
{
   auto milseconds = std::chrono::duration_cast<std::chrono::milliseconds >( _timePoint.time_since_epoch() );
   std::time_t t = std::chrono::duration_cast<std::chrono::seconds >( milseconds ).count();
   return t;
}

long long TimeUtil::unixTimeSeconds()
{
   auto seconds = std::chrono::duration_cast<std::chrono::seconds >( _timePoint.time_since_epoch() );
   return seconds.count();
}

long long TimeUtil::unixTimeMiliSeconds()
{
   auto milseconds = std::chrono::duration_cast<std::chrono::milliseconds >( _timePoint.time_since_epoch() );
   return milseconds.count();
}


} // namespace tbs