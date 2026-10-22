#pragma once

#include "tobasalis/exp_imp.h"

namespace tbs {
namespace lis {

/** \ingroup LIS
 * Delimiter
 * 
 */
struct TBS_LIS_API Delimiter
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