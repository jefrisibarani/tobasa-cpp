#pragma once

#include <string>
#include "tobasa/exp_imp.h"

namespace tbs {
namespace util {

/** 
 * @addtogroup TBS
 * @{
 */

/// Check UTF-8 Valid.
TBS_API bool isValidUtf8(const std::string &str);

/// Convert UTF-8 string to wstring.
TBS_API std::wstring utf8_to_wstring(const std::string &str);

/// Convert wstring to UTF-8 string.
TBS_API std::string wstring_to_utf8(const std::wstring &wstr);

/** @}*/

} // namespace util
} // namespace tbs
