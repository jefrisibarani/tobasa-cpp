#include <utf8cpp/utf8.h>
#include "tobasa/util_utf.h"

namespace tbs {
namespace util {

// Note: https://codingtidbit.com/2020/02/09/c17-codecvt_utf8-is-deprecated/

bool isValidUtf8(const std::string &str)
{
   return utf8::is_valid(str.begin(), str.end());
}

std::wstring utf8_to_wstring(const std::string &str)
{
   std::wstring wideStr;

   if (str.empty())
      return wideStr;

#ifdef _MSC_VER
   utf8::utf8to16(str.begin(), str.end(), std::back_inserter(wideStr));
#else
   utf8::utf8to32(str.begin(), str.end(), std::back_inserter(wideStr));
#endif
   return wideStr;
}

std::string wstring_to_utf8(const std::wstring &wstr)
{
   std::string stdStr;

   if (wstr.empty())
      return stdStr;

#ifdef _MSC_VER
   utf8::utf16to8(wstr.begin(), wstr.end(), std::back_inserter(stdStr));
#else
   utf8::utf32to8(wstr.begin(), wstr.end(), std::back_inserter(stdStr));
#endif
   return stdStr;
}

} // namespace util
} // namespace tbs
