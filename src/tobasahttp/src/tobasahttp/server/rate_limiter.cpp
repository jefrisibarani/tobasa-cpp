#include <iostream>
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
      std::cout << "IP " << ip << " is permanently blacklisted." << std::endl;
      return false;
   }

   if (_temporaryBlocklist.find(ip) != _temporaryBlocklist.end()) 
   {
      if (now < _temporaryBlocklist[ip]) 
      {
         std::cout << "IP " << ip << " is temporarily blocked." << std::endl;
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
   std::cout << "IP " << ip << " exceeded the rate limit." << std::endl;

   _violationCount[ip]++;
   if (_violationCount[ip] >= _maxViolations ) 
   {
      _blacklist.insert(ip);
      std::cout << "IP " << ip << " is permanently blacklisted." << std::endl;
   } 
   else 
   {
      _temporaryBlocklist[ip] = now + _blockDuration;
      std::cout << "IP " << ip << " is temporarily blocked for "
                << duration_cast<seconds>(_blockDuration).count() << " seconds." << std::endl;
   }
}


} // namespace http
} // namespace tbs