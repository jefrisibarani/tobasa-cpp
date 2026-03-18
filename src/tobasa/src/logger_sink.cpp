#include <iostream>
#include <thread>
#include <sstream>
#include <chrono>
#include "tobasa/format.h"
#include "tobasa/logger_sink.h"
#include "tobasa/util_time.h"

namespace tbs {
namespace log {

namespace {
std::string threadId() 
{
   std::stringstream ss;
   ss << std::this_thread::get_id();
   return ss.str();
}
}

CoutLogSink::CoutLogSink()
   : _out { &std::cout } {}

CoutLogSink::~CoutLogSink() {}

void CoutLogSink::doLog(const std::string& tag, const std::string& msg)
{
   std::lock_guard<std::mutex> lock{ _lock };

#if defined(TOBASA_USE_STD_FORMAT)

   using namespace std::chrono;
   auto timePoint = floor<milliseconds>(system_clock::now());
   auto zone = current_zone();
   auto timePointLocal = zone->to_local(timePoint);

   ( *_out )
      << tbsfmt::format(
            "[{:%Y%m%d_%H%M%S}.{:03d}] [{}] {} {}",
            timePointLocal,
            static_cast< int >( timePointLocal.time_since_epoch().count() % 1000u ),
            threadId(),
            tag,
            msg )
      << std::endl;
#else
   TimeUtil time;

   ( *_out )
      << tbsfmt::format(
            "[{:%Y%m%d_%H%M%S}.{:03d}] [{}] {} {}",
            time.localTime(),
            static_cast< int >( time.unixTimeMiliSeconds() % 1000u ),
            threadId(),
            tag,
            msg )
      << std::endl;
#endif
}

/// Log Trace message
void CoutLogSink::trace(const std::string& message)
{
   doLog("[trace]", message);
}

void CoutLogSink::debug(const std::string& message)
{
   doLog("[debug]", message);
}

void CoutLogSink::info(const std::string& message)
{
   doLog("[info]", message);
}

void CoutLogSink::warn(const std::string& message)
{
   doLog("[warn]", message);
}

void CoutLogSink::error(const std::string& message)
{
   doLog("[error]", message);
}

} // namespace log
} // namespace tbs