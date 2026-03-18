#pragma once

#include <string>

namespace tbs {
namespace test {

// Note:
// in c++ 20,  u8 literals is char8_t
// https://stackoverflow.com/a/56833374
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1423r2.html
inline std::string u8ToString(const std::string& s)
{
   return s;
}

inline std::string u8ToString(std::string&& s)
{
   return std::move(s);
}

#if defined(__cpp_lib_char8_t)

inline std::string u8ToString(const std::u8string& s)
{
   return std::string(s.begin(), s.end());
}

inline std::string operator"" _asStr(const char8_t* str, std::size_t)
{
   return reinterpret_cast< const char* >(str);
}

inline char const* operator"" _asChar(const char8_t* str, std::size_t)
{
   return reinterpret_cast< const char* >(str);
}

#else // !defined(__cpp_lib_char8_t)

inline std::string operator"" _asStr(const char* str, std::size_t)
{
   return str;
}

inline char const* operator"" _asChar(const char* str, std::size_t)
{
   return str;
}

#endif // defined(__cpp_lib_char8_t)



#if defined(__cpp_char8_t)
   #define U8(x) u8##x##_asChar
#else
   #define U8(x) u8##x
#endif

} // namespace test
} // namespace tbs
