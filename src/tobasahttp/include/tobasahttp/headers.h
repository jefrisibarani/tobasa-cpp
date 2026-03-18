#pragma once

#include "tobasahttp/field.h"
#include "tobasahttp/header_names.h"

namespace tbs {
namespace http {

/** 
 * \ingroup HTTP
 * \brief Request/Response headers
 */
class Headers
   : public FieldLowerCaseCollection
{
public:
   Headers() = default;
   virtual ~Headers() = default;
   
   FieldPtr field(size_t index)
   {
      if ( _fields.size() > 0 && index < _fields.size() )
         return _fields.at(index);
      else
         return nullptr;
   }

   FieldPtr field(HeaderName code)
   {
      auto name = headerNameToString(code);
      return find(name);
   }

};

} // namespace http
} // namespace tbs