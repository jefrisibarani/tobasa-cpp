#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <algorithm>
#include <cctype>

#include "matcher_interface.h"

namespace tbs {

/**
 * @class WildcardMatcher
 * @brief Wildcard-style string matcher with optional case sensitivity.
 *
 * Supports wildcards:
 * - `*` matches zero or more characters
 * - `?` matches exactly one character
 *
 * Examples:
 * - Pattern: "http*://localhost" -> matches both "http://localhost" and "https://localhost"
 * - Pattern: "*" -> matches anything
 *
 * Thread-safe and singleton-based. Implements IMatcher interface.
 */
class WildcardMatcher : public IMatcher
{
public:

   /// @brief Get the shared singleton instance.
   static WildcardMatcher& instance()
   {
      static WildcardMatcher inst;
      return inst;
   }

   /**
    * @brief Check if the text matches the pattern (supports * and ?).
    * @param text The input string.
    * @param pattern The pattern string.
    * @param caseSensitive True for case-sensitive matching.
    * @return True if match succeeds, false otherwise.
    */
   bool match(const std::string& text,
              const std::string& pattern,
              bool caseSensitive = false) override
   {
      if (pattern == "*")
         return true;

      std::string t = caseSensitive ? text : toLower(text);
      std::string p = caseSensitive ? pattern : toLower(pattern);

      return wildcardMatch(t, p);
   }

   /// @brief Clear cached lowercase conversions.
   void clearCache() override
   {
      std::unique_lock lock(_mutex);
      _cache.clear();
   }

private:
   WildcardMatcher() = default;

   /// @brief Convert to lowercase and cache result.
   const std::string& toLower(const std::string& s)
   {
      {
         std::shared_lock lock(_mutex);
         auto it = _cache.find(s);
         if (it != _cache.end())
            return it->second;
      }

      std::string lower = s;
      std::transform(lower.begin(), lower.end(), lower.begin(),
                     [](unsigned char c) { return std::tolower(c); });

      {
         std::unique_lock lock(_mutex);
         auto [it, inserted] = _cache.emplace(s, std::move(lower));
         return it->second;
      }
   }

   /// @brief Wildcard matching algorithm (greedy, case already handled).
   static bool wildcardMatch(const std::string& text, const std::string& pattern)
   {
      size_t t = 0, p = 0, star = std::string::npos, match = 0;

      while (t < text.size())
      {
         if (p < pattern.size() &&
             (pattern[p] == '?' || pattern[p] == text[t]))
         {
            ++p;
            ++t;
         }
         else if (p < pattern.size() && pattern[p] == '*')
         {
            star = p++;
            match = t;
         }
         else if (star != std::string::npos)
         {
            p = star + 1;
            t = ++match;
         }
         else
         {
            return false;
         }
      }

      while (p < pattern.size() && pattern[p] == '*')
         ++p;

      return p == pattern.size();
   }

private:
   std::unordered_map<std::string, std::string> _cache;
   std::shared_mutex _mutex;
};

} // namespace tbs