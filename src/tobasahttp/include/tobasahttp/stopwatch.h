#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/** 
 * Stopwatch class.

   usage:
   StopWatch sw;
   sw.start();
   // parse headers
   sw.lap(1, "headers");
   // read body
   sw.lap(1, "body");
   // write to disk
   sw.lap(1, "disk");
   // send response
   sw.lap("response");
   sw.stop();
   std::cout << "Total: " << sw.toString(true) << "\n";
   std::cout << "Detail: " << sw.report(true) << "\n";
 * 
 */
class StopWatch
{
public:
   using TimePointSteady = std::chrono::steady_clock::time_point;

   StopWatch() = default;
   ~StopWatch() = default;

   void start();
   void stop();
   int64_t result(uint32_t parsingId=0);
   int64_t resultMicros(uint32_t parsingId=0);
   void lap(uint32_t parsingId, const std::string& label);
   std::string toString(uint32_t parsingId, bool withSpace = false) const;
   std::string report(uint32_t parsingId, bool withSpace = false) const;

private:
   TimePointSteady _start{};
   TimePointSteady _end{};
   TimePointSteady _last{};
   std::vector<std::tuple<uint32_t, std::string, int64_t>> _laps; // parsingId, label, ms
};

/** @}*/

} // namespace http
} // namespace tbs