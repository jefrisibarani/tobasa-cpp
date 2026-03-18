
#include <random>
#include <sstream>
#include <ctime>
#include <cstdint>
#include <string>
#include <cstdlib>
#include <climits>
#include <cctype>

#include "tobasa/util.h"
#include "tobasa/util_string.h"
#include "tobasa/id_generator.h"

namespace tbs {
namespace util {

std::string generateUniqueId()
{
   IdGenerator gen;
   return gen.getId();
}

std::string getRandomString(size_t length)
{
   static const std::string chars =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";

   // thread_local ensures each thread has its own RNG instance
   thread_local std::mt19937_64 rng([] {
      // Try to seed with real entropy first
      std::random_device rd;
      if (rd.entropy() > 0) {
         return std::mt19937_64(rd());
      } 
      else 
      {
         // Fallback: use high-res clock if random_device is deterministic
         return std::mt19937_64(
            std::chrono::steady_clock::now().time_since_epoch().count()
         );
      }
   }());

   std::uniform_int_distribution<size_t> distribution(0, chars.size() - 1);

   std::string randomString;
   randomString.reserve(length); // efficiency
   for (size_t i = 0; i < length; ++i) {
      randomString += chars[distribution(rng)];
   }

   return randomString;
}

std::string getRandomNumber( size_t length )
{
   // Note: https://stackoverflow.com/a/12468109
   
   std::string chars("0123456789");
   std::string randomString;
   
   // Seed the random number generator with the current time
   std::mt19937_64 rng(std::time(0));

   // Create a uniform distribution for random indices
   std::uniform_int_distribution<int> distribution(0, chars.size() - 1);

   for (int i = 0; i < length; ++i) {
      int randomIndex = distribution(rng);
      randomString += chars[randomIndex];
   }

   return randomString;
}

std::string threadId(std::thread::id tid)
{
   std::stringstream ss;
   ss << tid;
   return ss.str();
}

std::string readMilliseconds(long long milliseconds)
{
   // Note: https://stackoverflow.com/a/50727882

   auto seconds = milliseconds / 1000;
   milliseconds %= 1000;

   auto minutes = seconds / 60;
   seconds %= 60;

   auto hours = minutes / 60;
   minutes %= 60;

   auto days = hours / 24;
   hours %= 24;

   std::string result;

   if (days > 0)
      result += std::to_string(days) + " d ";

   if (hours > 0)
      result += std::to_string(hours) + " h ";

   if (minutes > 0)
      result += std::to_string(minutes) + " m ";

   if (seconds > 0)
      result += std::to_string(seconds) + " s ";

   if (milliseconds > 0)
      result += std::to_string(milliseconds) + " ms";

   return result;
}

int64_t parseSizeInBytes(const std::string& value)
{
   if (value.size() < 2)
      return -1;

   std::string s = value;
   for (auto& c : s) c = std::toupper(c);

   int64_t factor = 1;

   if      ( util::endsWith(s, "KB") ) factor = 1024LL;
   else if ( util::endsWith(s, "MB") ) factor = 1024LL * 1024LL;
   else if ( util::endsWith(s, "GB") ) factor = 1024LL * 1024LL * 1024LL;
   else if ( util::endsWith(s, "TB") ) factor = 1024LL * 1024LL * 1024LL * 1024LL;
   else if ( util::endsWith(s,  "B") ) factor = 1LL;
   else return -1;

   // if      (s.ends_with("KB")) factor = 1024LL;
   // else if (s.ends_with("MB")) factor = 1024LL * 1024LL;
   // else if (s.ends_with("GB")) factor = 1024LL * 1024LL * 1024LL;
   // else if (s.ends_with("TB")) factor = 1024LL * 1024LL * 1024LL * 1024LL;
   // else if (s.ends_with( "B")) factor = 1LL;
   // else return -1;

   size_t pos = s.find_first_not_of("0123456789");
   if (pos == std::string::npos)
      return -1;

   std::string numPart = s.substr(0, pos);
   int64_t number = std::atoll(numPart.c_str());

   if (number <= 0)
      return -1;

   if (number > 0 && factor > INT64_MAX / number)
      return INT64_MAX;

   return number * factor;
}


} // namespace util
} // namespace tbs