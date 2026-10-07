#include "tobasahttp/headers.h"
#include <cctype>

namespace tbs {
namespace http {

HeaderValue Headers::findHeader(std::string_view name)
{
   HeaderValue result;
   for (size_t i = 0; i < this->size(); ++i)
   {
      auto field = this->field(i);
      if (!field)
         continue;

      auto fieldName = field->nameRef();
      if (fieldName.size() != name.size())
         continue;

      bool equal = true;
      for (size_t j = 0; j < name.size(); ++j)
      {
         if (std::tolower(static_cast<unsigned char>(fieldName[j])) !=
             std::tolower(static_cast<unsigned char>(name[j])))
         {
            equal = false;
            break;
         }
      }

      if (equal)
      {
         ++result.count;
         if (result.count == 1)
            result.value = field->value();
      }
   }
   return result;
}


} // namespace http
} // namespace tbs