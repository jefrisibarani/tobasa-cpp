#pragma once

#include "rule.h"
#include <string_view>
#include <unordered_map>
#include <functional>

namespace tbs {
namespace http {

using ValidatorFn = std::function<bool(std::string_view)>;

inline bool validateInt(std::string_view s)
{
   if (s.empty()) return false;
   for (unsigned char c : s)
   {
      if (!rule::isDigit(c)) return false;
   }
   return true;
}

inline bool validateAlpha(std::string_view s)
{
   if (s.empty()) return false;
   for (unsigned char c : s)
   {
      if (!rule::isAlpha(c)) return false;
   }
   return true;
}

inline bool validateAlnum(std::string_view s)
{
   if (s.empty()) return false;
   for (unsigned char c : s)
   {
      if (!(rule::isAlpha(c) || rule::isDigit(c)))
         return false;
   }
   return true;
}

inline bool validateHex(std::string_view s)
{
   if (s.empty()) return false;
   for (unsigned char c : s)
   {
      if (!rule::isHex(c)) return false;
   }
   return true;
}

// UUID: 8-4-4-4-12 hex chars with dashes
inline bool validateUuid(std::string_view s)
{
   if (s.size() != 36) return false;
   auto checkHex = [](std::string_view part) {
      if (part.empty()) return false;
      for (unsigned char c : part)
      {
         if (!rule::isHex(c)) return false;
      }
      return true;
   };

   return checkHex(s.substr(0,8))  &&
          s[8]  == '-'             &&
          checkHex(s.substr(9,4))  &&
          s[13] == '-'             &&
          checkHex(s.substr(14,4)) &&
          s[18] == '-'             &&
          checkHex(s.substr(19,4)) &&
          s[23] == '-'             &&
          checkHex(s.substr(24,12));
}

// slug: lowercase letters, digits, dash, underscore
inline bool validateSlug(std::string_view s)
{
   if (s.empty()) return false;
   for (unsigned char c : s)
   {
      if (!(rule::isAlphaLow(c) || rule::isDigit(c) || c == '-' || c == '_'))
         return false;
   }
   return true;
}


/**
   Safe default for text segments
   Rejects empty values
   Rejects . and .. (path traversal)
   Rejects / (segment breaker, should never appear in one part)
   Rejects control chars (rule::isCtl)
 */
inline bool validateString(std::string_view s)
{
   if (s.empty()) return false;
   if (s == "." || s == "..") return false; // reject traversal
   for (unsigned char c : s)
   {
      if (c == '/' || rule::isCtl(c))
         return false;
   }
   return true;
}

/*
   int   → only digits
   alpha → only letters
   alnum → letters + digits
   hex   → hex digits only
   uuid  → RFC4122 UUID format
   slug  → lowercase, digits, dash, underscore
   string
   
*/
inline const std::unordered_map<std::string_view, ValidatorFn>& getValidators()
{
   static const std::unordered_map<std::string_view, ValidatorFn> validators {
      {"int",    validateInt},
      {"alpha",  validateAlpha},
      {"alnum",  validateAlnum},
      {"hex",    validateHex},
      {"uuid",   validateUuid},
      {"slug",   validateSlug},
      {"string", validateString} // default safe text
   };
   return validators;
}

} // namespace http
} // namespace tbs
