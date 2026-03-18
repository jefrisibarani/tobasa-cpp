#include <chrono>
#include "tobasa/util_string.h"

namespace tbs {
namespace util {

namespace {

inline char upcase(char c)
{
   return (c >= 'a' && c <= 'z') ? (c & ~0x20) : c;
}
inline char lowcase(char c)
{
   return (c >= 'A' && c <= 'Z') ? (c | 0x20) : c;
}

}


/** \addtogroup TBS
 * @{
 */

std::string_view trimDoubleQuote(std::string_view s)
{
   if ( s.front() == '"')
      s.remove_prefix(1);

   if (s.back() == '"')
      s.remove_suffix(1);

   return s;
}

std::string toLower(std::string s)
{
   std::transform(s.begin(), s.end(), s.begin(), lowcase);
   return std::move(s);
}

void strLower(std::string& s)
{
   std::transform(s.begin(), s.end(), s.begin(), lowcase );
}

std::string toUpper(std::string s)
{
   std::transform(s.begin(), s.end(), s.begin(), upcase );
   return std::move(s);
}

void strUpper(std::string& s)
{
   std::transform(s.begin(), s.end(), s.begin(), upcase );
}

std::string replace(const std::string& str, const std::string& from, const std::string& to)
{
   std::string out = { str };

   size_t startPos = str.find(from);
   if (startPos == std::string::npos)
      return str;

   out.replace(startPos, from.length(), to);
   return out;
}

void replaceAll(std::string &str, const std::string& from, const std::string& to)
{
   // Note: https://stackoverflow.com/a/24315631

   size_t startPos = 0;
   while ((startPos = str.find(from, startPos)) != std::string::npos) 
   {
      str.replace(startPos, from.length(), to);
      startPos += to.length(); // Handles case where 'to' is a substring of 'from'
   }
}

std::string trim(const std::string& s)
{
   auto front = std::find_if_not(s.begin(), s.end(),
                     [](int c) {return std::isspace(c); });

   auto back  = std::find_if_not(s.rbegin(), s.rend(),
                     [](int c) {return std::isspace(c); }).base();

   return (back <= front ? std::string {} : std::string{front, back} );
}

std::string trim(std::string_view s)
{
   return trim(std::string{s});
}

bool startsWith(const std::string& str, const std::string& other)
{
   return str.size() >= other.size() && str.compare( 0, other.size(), other ) == 0;
}

bool startsWith(std::string_view str, std::string_view other)
{
   return str.size() >= other.size() && str.compare( 0, other.size(), other ) == 0;
}

bool endsWith(std::string_view str, std::string_view other ) noexcept
{
   return str.size() >= other.size() && 0 == str.compare( str.size() - other.size(), other.size(), other);
}

bool contains(const std::string& str, const std::string& searchVal) {
   return str.find(searchVal) != std::string::npos;
}

std::vector<std::string> split(const char* str, char delimiter)
{
   // Note: https://stackoverflow.com/a/53878

   std::vector<std::string> result;

   do
   {
      const char* begin = str;

      while (*str != delimiter && *str)
         str++;

      result.push_back(std::string(begin, str));
   } while (0 != *str++);

   return result;
}

std::vector<std::string> split(const std::string& s, const std::string& delim, const bool keepEmpty)
{
   // Note: https://stackoverflow.com/a/236180

   std::vector<std::string> result;
   if (delim.empty())
   {
      result.push_back(s);
      return result;
   }

   std::string::const_iterator substart = s.begin(), subend;

   while (true)
   {
      subend = std::search(substart, s.end(), delim.begin(), delim.end());
      std::string temp(substart, subend);
      if (keepEmpty || !isOnlyWhiteSpace(temp))
      {
         result.push_back(temp);
      }

      if (subend == s.end())
         break;

      substart = subend + delim.size();
   }

   return result;
}

std::vector<std::string> split(const std::string& s, char delim, const bool keepEmpty)
{
   std::string delimiter{delim};
   return split(s, delimiter, keepEmpty);
}

bool isNumber(const std::string& str)
{
   if (str.empty())
      return false;

   return str.find_first_not_of("0123456789") == std::string::npos;
}

bool numToBool(int value) { return value > 0; }

std::string boolToStr(bool value, std::string valIfTrue, std::string valIfFalse)
{
   return value ? valIfTrue : valIfFalse;
}

bool strToBool(const std::string& value)
{
   if (value.empty())
      return false;

   auto valueLowCase = toUpper(value);

   if (  valueLowCase == "T"
      || valueLowCase == "TRUE"
      || valueLowCase == "ON"
      || valueLowCase == "Y"
      || startsWith(value, "Y")
      || startsWith(value, "y")
      || startsWith(value, "1"))
   {
      return true;
   }

   return false;
}

bool removeWhiteSpace(std::string& str)
{
   // Note: https://stackoverflow.com/a/6319888

   size_t i = 0;
   size_t len = str.length();
   while(i < len)
   {
      if (!isalnum(str[i]) || str[i] == ' ')
      {
         str.erase(i,1);
         len--;
      }
      else
         i++;
   }

   return true;
}

bool removeTrailingWhiteSpace(std::string& data)
{
   size_t len = data.length();
   size_t i = 0;
   while (len > 0)
   {
      i = len - 1;
      auto ch = data.at(i);

      if ( (ch >= 0 && ch <= 31) || (ch == 127) ||  // CTL
           (ch == ' ' || ch == '\t')  )             // SPACE and TAB
      {
         data.erase(i, 1);
      }
      else
      {
         // stop if we found printable chars
         break;
      }

      len--;
   }

   return true;
}

bool removeTraillingChar(std::string& data, char ch)
{
   size_t len = data.length();
   size_t i = 0;
   while (len > 0)
   {
      i = len - 1;
      auto c = data.at(i);

      if ( c == ch  )
         data.erase(i, 1);
      else {
         break;
      }

      len--;
   }

   return true;
}

bool isOnlyWhiteSpace(const std::string& str)
{
   // // If a character other than space, tab, or newline is found, return false
   // \t\n\v\f\r
   for (char c : str) 
   {
      if (c != ' ' && 
          c != '\t' && 
          c != '\n' && 
          c !=  'r' ) 
      {
         return false;
      }
   }
   return true;
}

void parseNameValue(const std::string& line, std::function<void(const std::string,const std::string)> handler )
{
   auto pos = line.find_first_of('=');
   if ( pos != std::string::npos )
   {
      auto val  = line.substr(pos+1);
      auto name = line.substr(0, (line.size()-1) - val.size() );
      handler(name,val);
   }
}

/** @}*/

} // namespace util
} // namespace tbs
