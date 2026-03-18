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
 * @class RateLimiter
 * @brief A class that limits the rate of incoming requests from different IP addresses.
 *
 * The RateLimiter enforces a limit on the number of requests an IP can make within a specified
 * time window. If an IP exceeds the allowed number of requests, it can be temporarily or 
 * permanently blocked based on the violation history.
 */
class RateLimiter 
{
public:
   /**
    * @brief Constructs a RateLimiter with the specified rate-limiting parameters.
    * 
    * @param maxRequests Maximum allowed requests per time window.
    * @param windowDuration The time window during which requests are counted.
    * @param blockDuration The duration for which an IP is temporarily blocked after exceeding the limit.
    * @param maxViolations The number of violations allowed before permanently blacklisting an IP.
    */
   RateLimiter(int maxRequests, milliseconds windowDuration, milliseconds blockDuration, int maxViolations);

   /**
    * @brief Checks if a request from the given IP is allowed.
    *
    * This method checks if the IP is allowed to make a request based on the rate limit
    * and whether the IP is temporarily or permanently blocked.
    *
    * @param ip The IP address making the request.
    * @return true if the request is allowed, false if the IP is blocked or has exceeded the rate limit.
    */
   bool allowRequest(const std::string &ip);

private:

   /**
    * @brief Handles an IP's violation of the request limit.
    *
    * This method is called when an IP exceeds the allowed number of requests within the time window.
    * It either temporarily blocks the IP or permanently blacklists it if the number of violations exceeds the maximum allowed.
    *
    * @param ip The IP address that violated the request limit.
    * @param now The current time, used to track when the violation occurred.
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
};

/** @}*/

} // namespace http
} // namespace tbs