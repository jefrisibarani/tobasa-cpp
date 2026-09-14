#include <iostream>
#include <tobasa/logger.h>
#include "tobasahttp/server/rate_limiter.h"


namespace tbs {
namespace http {

RateLimiter::RateLimiter(int32_t maxRequests, milliseconds windowDuration, milliseconds blockDuration, int32_t maxViolations)
   : _maxRequests(maxRequests),
     _windowDuration(windowDuration),
     _blockDuration(blockDuration),
     _maxViolations(maxViolations) {}

bool RateLimiter::allowRequest(const std::string &ip)
{
   auto now = steady_clock::now();

   if (_blacklist.find(ip) != _blacklist.end()) 
   {
      Logger::logW("[{}] IP {} is permanently blacklisted.", _logHttpType, ip);
      return false;
   }

   if (_temporaryBlocklist.find(ip) != _temporaryBlocklist.end()) 
   {
      if (now < _temporaryBlocklist[ip]) 
      {
         Logger::logW("[{}] IP {} is temporarily blocked.", _logHttpType, ip);
         return false;
      } 
      else 
      {
         _temporaryBlocklist.erase(ip);
      }
   }

   auto &requestTimes = _requestLog[ip];

   while (!requestTimes.empty() && now - requestTimes.front() > _windowDuration) 
   {
      requestTimes.pop_front();
   }

   if (requestTimes.size() >= static_cast<size_t>(_maxRequests) ) 
   {
      handleViolation(ip, now);
      return false;
   }

   requestTimes.push_back(now);
   return true;
}

void RateLimiter::handleViolation(const std::string &ip, const steady_clock::time_point &now)
{
   Logger::logW("[{}] IP {} is exceeded the rate limit.", _logHttpType, ip);

   _violationCount[ip]++;
   if (_violationCount[ip] >= _maxViolations ) 
   {
      _blacklist.insert(ip);
      Logger::logW("[{}] IP {} is permanently blacklisted.", _logHttpType, ip);
   } 
   else 
   {
      _temporaryBlocklist[ip] = now + _blockDuration;
      Logger::logW("[{}] IP {} is temporarily blocked for {} seconds.", _logHttpType, ip, duration_cast<seconds>(_blockDuration).count());
   }
}


} // namespace http
} // namespace tbs