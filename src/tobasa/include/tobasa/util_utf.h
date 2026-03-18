#pragma once

#include <string>

namespace tbs {
namespace util {

/** @addtogroup TBS
 * @{
 */

/// Check UTF-8 Valid
bool isValidUtf8(const std::string &str);

/// Convert UTF-8 string to wstring
std::wstring utf8_to_wstring(const std::string &str);

/// Convert wstring to UTF-8 string
std::string wstring_to_utf8(const std::wstring &wstr);

/** @}*/

} // namespace util
} // namespace tbs
