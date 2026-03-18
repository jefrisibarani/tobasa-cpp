#pragma once

namespace tbs {
namespace lis {

/** \ingroup LIS
 * Delimiter
 * 
 */
struct Delimiter
{
   char fieldDelimiter     = '\x00';   // '|'
   char repeatDelimiter    = '\x00';   // '\\'
   char componentDelimiter = '\x00';   // '!'
   char escapeCharacter    = '\x00';   // '~'

   bool initialized()
   {
      return fieldDelimiter && repeatDelimiter && componentDelimiter && escapeCharacter;
   }

};

} // namespace lis
} // namespace tbs