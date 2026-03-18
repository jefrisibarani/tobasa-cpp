#pragma once

#include <iostream>
#include <vector>
#include <string_view>
#include <optional>
#include "tobasahttp/headers.h"
#include "tobasahttp/field.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/** 
 * Http uri query string.
 * \sa https://www.rfc-editor.org/rfc/rfc3986#section-3.3
 * field1=value1&field2=value2&field3=value3... *
 */
class QueryString
   : public FieldCollection
{
public:
   QueryString() = default;
   ~QueryString() = default;

   void parse(const std::string& text);

   /**
    * Get parameter value.
    * \param name parameter name
    */
   std::optional<std::string> get(const std::string& name) const;

   /**
    * Get parameter by position.
    * \param position zero based index
    */
   std::optional<FieldPtr> get(int32_t position) const;
};

using QueryStringPtr = std::shared_ptr<QueryString>;

/** @}*/

} // namespace http
} // namespace tbs