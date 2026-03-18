#include <sstream>
#include <tuple>
#include "tobasahttp/stopwatch.h"

namespace tbs {
namespace http {

std::string timeIntervalToString(int64_t micros, bool withSpace) 
{
   auto milliseconds = micros / 1000;
   auto microseconds = micros % 1000;

   auto seconds = milliseconds / 1000;
   milliseconds %= 1000;

   auto minutes = seconds / 60;
   seconds %= 60;

   auto hours = minutes / 60;
   minutes %= 60;

   std::string result;
   std::string space = withSpace ? " " : "";

   if (hours        > 0) result += std::to_string(hours)        + "h"  + space;
   if (minutes      > 0) result += std::to_string(minutes)      + "m"  + space;
   if (seconds      > 0) result += std::to_string(seconds)      + "s"  + space;
   if (milliseconds > 0) result += std::to_string(milliseconds) + "ms" + space;
   if (microseconds > 0) result += std::to_string(microseconds) + "us";

   return result;
}


void StopWatch::start()
{
   _start = std::chrono::steady_clock::now();
   _last = _start;
   _laps.clear();
}

void StopWatch::stop()
{
   _end = std::chrono::steady_clock::now();
}

int64_t StopWatch::result(uint32_t parsingId)
{
   using namespace std::chrono;
   if (parsingId == 0)
   {
      auto endPoint = (_end == TimePointSteady{}) ? steady_clock::now() : _end;
      return duration_cast<milliseconds>(endPoint - _start).count();
   }
   else
   {
      int64_t start,end;
      start = end = 0;  
      for (const auto& lap : _laps)
      {
         if (std::get<0>(lap) == parsingId)
         {
            if (std::get<1>(lap) == "start")
               start = std::get<2>(lap);
            else if (std::get<1>(lap) == "stop")
               end = std::get<2>(lap);
         }
      }

      if (end == 0)
      {
         auto now = std::chrono::steady_clock::now();
         end = duration_cast<milliseconds>(now - _start).count();
      }
      return (end-start)/1000;
   }
}

int64_t StopWatch::resultMicros(uint32_t parsingId)
{
   using namespace std::chrono;
   if (parsingId == 0)
   {
      auto endPoint = (_end == TimePointSteady{}) ? steady_clock::now() : _end;
      return duration_cast<microseconds>(endPoint - _start).count();
   }
   else
   {
      int64_t start,end;
      start = end = 0;  
      for (const auto& lap : _laps)
      {
         if (std::get<0>(lap) == parsingId)
         {
            if (std::get<1>(lap) == "start")
               start = std::get<2>(lap);
            else if (std::get<1>(lap) == "stop")
               end = std::get<2>(lap);
         }
      }

      if (end == 0)
      {
         auto now = std::chrono::steady_clock::now();
         end = duration_cast<microseconds>(now - _start).count();
      }
      return (end-start);
   }
}

void StopWatch::lap(uint32_t parsingId, const std::string& label)
{
   auto now = std::chrono::steady_clock::now();

   if (label == "start" || label == "stop")
   {
      auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - _start).count();
      _laps.emplace_back(parsingId, label, elapsed);
   }
   else
   {
      auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - _last).count();
      _last = now;
      _laps.emplace_back(parsingId, label, elapsed);
   }
}

std::string StopWatch::toString(uint32_t parsingId, bool withSpace) const
{
   auto micros = const_cast<StopWatch*>(this)->resultMicros(parsingId);

   return timeIntervalToString(micros, withSpace);
}


std::string StopWatch::report(uint32_t parsingId, bool withSpace) const
{
   std::ostringstream oss;
   std::string space = withSpace ? " " : "";

   for (size_t i = 0; i < _laps.size(); ++i) 
   {
      auto& lap = _laps[i];
      if (parsingId != 0 && std::get<0>(lap) != parsingId)
         continue;

      if (std::get<1>(lap) == "start" || std::get<1>(lap) == "stop")
         continue;   

      auto timeStr = timeIntervalToString(std::get<2>(lap),false);
      oss << std::get<1>(lap) << "=" << timeStr;
      //oss << std::get<1>(lap) << "=" << std::get<2>(lap) << "us";

      if (i + 1 < _laps.size()) 
         oss << space;
   }

   return oss.str();
}

} // namespace http
} // namespace tbs