#pragma once
#include <string>

namespace tbs {

/**
 * @interface IMatcher
 * @brief Abstract interface for all matchers (wildcard or regex).
 *
 * This interface defines the basic matching behavior:
 * - match() -> to test if a text matches a pattern
 * - clearCache() -> to reset any cached data
 *
 * Both WildcardMatcher and RegexPatternMatcher implement this interface.
 */
class IMatcher
{
public:
   virtual ~IMatcher() = default;

   /**
    * @brief Check if the text matches the given pattern.
    * @param text The input string to test.
    * @param pattern The pattern string.
    * @param caseSensitive If true, match is case-sensitive.
    * @return True if matched, false otherwise.
    */
   virtual bool match(const std::string& text,
                      const std::string& pattern,
                      bool caseSensitive = false) = 0;

   /// @brief Clear any cached compiled patterns or data.
   virtual void clearCache() = 0;
};

} // namespace tbs