#pragma once

#include <cctype>
#include <algorithm>
#include <string>
#include <vector>
#include <string_view>
#include <functional>
#include "tobasa/exp_imp.h"

namespace tbs {
namespace util {

/** @addtogroup TBS
 * @{
 */


/// Trim Double Quote from a string view 
TBS_API std::string_view trimDoubleQuote(std::string_view s);


/// Lowercases string.
template <typename T>
std::basic_string<T> toLower(const std::basic_string<T>& s)
{
   std::basic_string<T> s2 = s;
   std::transform(s2.begin(), s2.end(), s2.begin(), std::tolower);
   return std::move(s2);
}

/// Uppercases string.
template <typename T>
std::basic_string<T> toUpper(const std::basic_string<T>& s)
{
   std::basic_string<T> s2 = s;
   std::transform(s2.begin(), s2.end(), s2.begin(), std::toupper);
   return std::move(s2);
}

/// Lowercases string.
TBS_API std::string toLower(std::string s);
TBS_API void strLower(std::string& s);

/// Uppercases string.
TBS_API std::string toUpper(std::string s);
TBS_API void strUpper(std::string& s);

/// Replace part of a std::string.
TBS_API std::string replace(const std::string& str, const std::string& from, const std::string& to);

/// Replace part of a std::string.
TBS_API void replaceAll(std::string &str, const std::string& from, const std::string& to);

/// Trim a string.
TBS_API std::string trim(const std::string& s);

TBS_API std::string trim(std::string_view s);

/// Check if a string str starts with some other string.
TBS_API bool startsWith(const std::string& str, const std::string& other);

/// Check if a string_view str starts with some other string.
TBS_API bool startsWith(std::string_view str, std::string_view other);

TBS_API bool endsWith(std::string_view str, std::string_view other ) noexcept;

/// Check if a string str contains searchVal.
TBS_API bool contains(const std::string& str, const std::string& searchVal);

/// Split string (C String) using a char delimiter.
TBS_API std::vector<std::string> split(const char* str, char delimiter);

/// Split std::string using a std::string delimiter.
TBS_API std::vector<std::string> split(const std::string& s, const std::string& delim, const bool keepEmpty = true);

/// Split std::string using a char delimiter.
TBS_API std::vector<std::string> split(const std::string& s, char delim, const bool keepEmpty = true);

/// Check if a std::string is a unsigned number (integer).
TBS_API bool isNumber(const std::string& str);

/// Convert number(int) to boolean.
TBS_API bool numToBool(int value);

/// Convert boolean to std::string  "true" / "false".
TBS_API std::string boolToStr(bool value, std::string valIfTrue = "true", std::string valIfFalse = "false");

/// Convert std::string to bool.
TBS_API bool strToBool(const std::string& value);

/// Remove non apphabet and numeric and space from string.
TBS_API bool removeWhiteSpace(std::string& str);

/// Remove tarailing white space (including cr lf tab) from string.
TBS_API bool removeTrailingWhiteSpace(std::string& data);

/// Remove trailing char from string.
TBS_API bool removeTraillingChar(std::string& data, char ch);

TBS_API bool isOnlyWhiteSpace(const std::string& str);

/// Parse name and value from a string in format:  name=value.
TBS_API void parseNameValue(const std::string& tok, std::function<void(const std::string,const std::string)> handler );


template <typename InputIt1, typename InputIt2>
bool streq(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2) 
{
  if (std::distance(first1, last1) != std::distance(first2, last2)) {
    return false;
  }

  return std::equal(first1, last1, first2);
}

template <typename T, typename S> bool streq(const T &a, const S &b) 
{
  return streq(a.begin(), a.end(), b.begin(), b.end());
}

template <typename T, size_t N> constexpr size_t strSize(T (&)[N]) 
{
  return N - 1;
}

template <typename T, typename S>
bool streq(const T &a, const S &b, size_t blen) 
{
  return std::equal(std::begin(a), std::end(a), std::begin(b),
                    std::next(std::begin(b), blen));
}

TBS_API std::string doubleToString(double value);
TBS_API void doubleToString(double value, std::string& outStr);

TBS_API std::string floatToString(float value);
TBS_API void floatToString(float value, std::string& outStr);

/** @}*/
} // namespace util
} // namespace tbs
