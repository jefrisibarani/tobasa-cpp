#pragma once

#include <string>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <iterator>
#include <thread>
#include "tobasa/common.h"

namespace tbs {
namespace util {

/** @addtogroup TBS
 * @{
 */

/// Find position of an element in a vector.
template < typename T>
inline long findPositionInVector(const std::vector<T>& vector, const T& element)
{
   long position = tbs::NOT_FOUND;

   auto it = std::find(vector.begin(), vector.end(), element);
   if (it != vector.end())
   {
      position = static_cast<long>(std::distance(vector.begin(), it));
      return position;
   }

   return position;
}

/// Convert vector content to comma separated values.
template <typename T>
inline std::string vetorToCsvString(const std::vector<T>& vec)
{
   // Note: https://www.geeksforgeeks.org/transform-vector-string/

   if (vec.empty())
      return "";

   std::ostringstream vts;

   // Convert all but the last element to avoid a trailing ","
   std::copy(vec.begin(), vec.end() - 1,  std::ostream_iterator<T>(vts, ", "));

   // Now add the last element with no delimiter
   vts << vec.back();

   return vts.str();
}

std::string generateUniqueId();

std::string getRandomString( size_t length );

std::string getRandomNumber( size_t length );

std::string threadId(std::thread::id tid);

std::string readMilliseconds(long long milliseconds);

/**
 * @brief Parses a size string (e.g. "100GB") into bytes.
 *
 * Uses binary (1024-based) scaling.
 * Accepts only uppercase 'B' for bytes.
 * Valid units: B, KB, MB, GB, TB.
 *
 * @param value Size string to parse.
 * @return Value in bytes, or -1 on invalid input or overflow.
 */
int64_t parseSizeInBytes(const std::string& value);

/** @}*/

}} // namespace tbs::util
