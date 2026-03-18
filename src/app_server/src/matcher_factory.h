#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include <algorithm>
#include <cctype>

#include "matcher_interface.h"
#include "matcher_wildcard.h"
#include "matcher_regex.h"

namespace tbs {

/**
 * @class MatcherFactory
 * @brief Extensible factory for creating or registering matcher instances.
 *
 * Provides built-in matchers:
 * - "regex" -> RegexMatcher
 * - "wildcard" -> WildCardMatcher
 *
 * Also supports custom matcher registration at runtime:
 * @code
 *   class CIDRMatcher : public IMatcher { ... };
 *   MatcherFactory::registerType("cidr", std::make_shared<CIDRMatcher>());
 *   IMatcher& m = MatcherFactory::create("cidr");
 * @endcode
 */
class MatcherFactory
{
public:
   /**
    * @brief Get matcher instance by type string.
    * @param type Matcher type (e.g. "regex", "wildcard", "cidr").
    * @return Reference to an existing IMatcher instance.
    *
    * If the type is not found, defaults to WildCardMatcher.
    */
   static IMatcher& create(const std::string& type)
   {
      std::string lower = toLower(type);

      {
         std::shared_lock lock(_mutex);
         auto it = _registry.find(lower);
         if (it != _registry.end())
            return *it->second;
      }

      // Built-in types
      if (lower == "regex")
         return RegexMatcher::instance();

      return WildcardMatcher::instance();
   }

   /**
    * @brief Register a custom matcher.
    * @param type Type name (case-insensitive, e.g. "cidr").
    * @param matcher Shared pointer to a custom IMatcher instance.
    *
    * Thread-safe and can be called before or during runtime.
    */
   static void registerType(const std::string& type, std::shared_ptr<IMatcher> matcher)
   {
      std::unique_lock lock(_mutex);
      _registry[toLower(type)] = std::move(matcher);
   }

   /// @brief Unregister a custom matcher type.
   static void unregisterType(const std::string& type)
   {
      std::unique_lock lock(_mutex);
      _registry.erase(toLower(type));
   }

   /// @brief List all registered matcher types (for debugging or diagnostics).
   static std::vector<std::string> listRegistered()
   {
      std::shared_lock lock(_mutex);
      std::vector<std::string> names;
      for (const auto& [k, _] : _registry)
         names.push_back(k);

      return names;
   }

private:
   static std::string toLower(const std::string& s)
   {
      std::string lower = s;
      std::transform(lower.begin(), lower.end(), lower.begin(),
                     [](unsigned char c) { return std::tolower(c); });

      return lower;
   }

private:
   static inline std::unordered_map<std::string, std::shared_ptr<IMatcher>> _registry;
   static inline std::shared_mutex _mutex;
};

} // namespace tbs