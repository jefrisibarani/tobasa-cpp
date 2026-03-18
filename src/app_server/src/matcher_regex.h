#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <regex>

#include "matcher_interface.h"

namespace tbs {

/**
 * @class RegexMatcher
 * @brief Thread-safe regex matcher with caching and case sensitivity.
 *
 * Caches compiled regex patterns for performance.
 * Implements IMatcher interface.
 */
class RegexMatcher : public IMatcher
{
public:
   static RegexMatcher& instance()
   {
      static RegexMatcher inst;
      return inst;
   }

   bool match(const std::string& text,
              const std::string& pattern,
              bool caseSensitive = false) override
   {
      if (pattern == "*")
         return true;

      try
      {
         const std::regex& rx = getCachedRegex(pattern, caseSensitive);
         return std::regex_search(text, rx);
      }
      catch (const std::regex_error&)
      {
         return false;
      }
   }

   void clearCache() override
   {
      std::unique_lock lock(_mutex);
      _cache.clear();
   }

private:
   RegexMatcher() = default;

   const std::regex& getCachedRegex(const std::string& pattern, bool caseSensitive)
   {
      std::string key = pattern + (caseSensitive ? "|CS" : "|CI");

      {
         std::shared_lock lock(_mutex);
         auto it = _cache.find(key);
         if (it != _cache.end())
            return it->second;
      }

      std::regex compiled(pattern,
         caseSensitive ? std::regex::ECMAScript
                       : (std::regex::ECMAScript | std::regex::icase));

      {
         std::unique_lock lock(_mutex);
         auto [it, inserted] = _cache.emplace(key, std::move(compiled));
         return it->second;
      }
   }

private:
   std::unordered_map<std::string, std::regex> _cache;
   std::shared_mutex _mutex;
};

} // namespace tbs