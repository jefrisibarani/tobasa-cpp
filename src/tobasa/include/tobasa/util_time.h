#pragma once

#include <ctime>
#include <chrono>
#include <mutex>

namespace tbs {

class TimeUtil
{
private:
   std::chrono::system_clock::time_point _timePoint{std::chrono::system_clock::now()};
   std::mutex _lock;

public:
   std::tm utcTime();
   std::tm localTime();
   std::time_t time();
   long long unixTimeSeconds();
   long long unixTimeMiliSeconds();
};

} // namespace tbs