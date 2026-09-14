#pragma once

#include <unordered_map>
#include <chrono>
#include <deque>
#include <set>
#include <string>

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

using namespace std::chrono;

/**
 * \brief Limits request frequency per client IP.
 *
 * Tracks the number of requests from each IP within a time window.
 * If an IP exceeds the allowed limit, it may be temporarily blocked
 * or permanently blacklisted after repeated violations.
 */
class RateLimiter 
{
public:
   /**
    * @brief Creates a rate limiter.
    * @param maxRequests Maximum requests allowed in the time window.
    * @param windowDuration Time window used to count requests.
    * @param blockDuration How long an IP stays temporarily blocked.
    * @param maxViolations Number of violations before permanent blacklist.
    */
   RateLimiter(int maxRequests, milliseconds windowDuration, milliseconds blockDuration, int maxViolations);

   /**
    * @brief Returns true if the request from the given IP is allowed.
    * @param ip Client IP address.
    * @return true if the IP is within the limit and not blocked; otherwise false.
    */
   bool allowRequest(const std::string &ip);

   void logHttpType(const std::string& logType) {_logHttpType=logType;}

private:

   /**
    * @brief Applies the rate-limit violation for an IP.
    * @param ip Client IP address.
    * @param now Current time.
    */
   void handleViolation(const std::string &ip, const steady_clock::time_point &now);

   int _maxRequests;
   milliseconds _windowDuration;
   milliseconds _blockDuration;
   int _maxViolations;

   std::unordered_map<std::string, std::deque<steady_clock::time_point>> _requestLog;
   std::unordered_map<std::string, steady_clock::time_point> _temporaryBlocklist;
   std::unordered_map<std::string, int> _violationCount;
   std::set<std::string> _blacklist;

   std::string _logHttpType;
};

/** @}*/

} // namespace http
} // namespace tbs