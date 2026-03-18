#pragma once

#include <string_view>

namespace tbs {
namespace http {
namespace rule {

/** \addtogroup HTTP
 * @{
 */

// https://datatracker.ietf.org/doc/html/rfc5234#appendix-B.1

using namespace std::literals;

static constexpr char CR       {13}; // '\r'   hex: 0D
static constexpr char LF       {10}; // '\n'   hex: 0A
static constexpr char COLON    {58}; // Colon
static constexpr char SP       {32}; // single space
static constexpr char HTAB     {9};  // horizontal tab
static constexpr char PERCENT  {37}; // %
static constexpr char QUESTION {63}; // ?
static constexpr char DQUOTE   {34}; // "

inline bool isDigit(unsigned char c)
{
   return ('0' <= c && c <= '9');
}

inline bool isAlphaLow(unsigned char c)
{
   return ('a' <= c && c <= 'z');
}

inline bool isAlphaUp(unsigned char c)
{
   return ('A' <= c && c <= 'Z');
}

inline bool isAlpha(unsigned char c)
{
   return (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'));
}

inline bool isDelimiter(unsigned char c)
{
   return ("\"(),/:;<=>?@[\\]{}\t"sv.find_first_of(c) != std::string_view::npos);
}

inline bool isChar(unsigned char c)
{
   return (c >= 1 && c <= 126);
}

inline bool isVchar(unsigned char c)
{
   return (c >= 21 && c <= 126);
}

inline bool isCtl(unsigned char c)
{
   return (c >= 0 && c <= 31) || (c == 127);
}

inline bool isOws(unsigned char c)
{
   return (c == rule::SP || c == rule::HTAB);
}

inline bool isHex(unsigned char c)
{
   bool isHex = ((c >= '0') && (c <= '9')) ||
                ((c >= 'A') && (c <= 'F')) ||
                ((c >= 'a') && (c <= 'f'));

   return isHex;
}

/** @}*/

} // namespace rule
} // namespace http
} // namespace tbs