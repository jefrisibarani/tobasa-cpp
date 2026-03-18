#include "tobasa/util_string.h"
#include "tobasa/json.h"

namespace tbs {

std::string cleanJsonException(const Json::exception& ex)
{
   int id = ex.id;

   while (id >= 10) {
      id = id/10;
   }

   std::string clearMessage;
   std::string exceptionName;

   if (id==1)
   {
      exceptionName = "[json.exception.parse_error." + std::to_string(ex.id) + "] ";
      clearMessage = util::replace(ex.what(), exceptionName, "");
      return clearMessage;
   }
   else if (id==2)
   {
      exceptionName = "[json.exception.invalid_iterator." + std::to_string(ex.id) + "] ";
      clearMessage = util::replace(ex.what(), exceptionName, "");
      return clearMessage;
   }
   else if (id==3)
   {
      exceptionName = "[json.exception.type_error." + std::to_string(ex.id) + "] ";
      clearMessage = util::replace(ex.what(), exceptionName, "");
      return clearMessage;
   }
   else if (id==4)
   {
      exceptionName = "[json.exception.out_of_range." + std::to_string(ex.id) + "] ";
      clearMessage = util::replace(ex.what(), exceptionName, "");
      return clearMessage;
   }
   else
      return ex.what();
}

} // namespace tbs