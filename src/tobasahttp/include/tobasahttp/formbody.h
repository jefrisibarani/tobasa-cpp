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
 * \brief Html Form Data.
 */
class FormBody
   : public FieldLowerCaseCollection
{
public:
   FormBody() = default;
   virtual ~FormBody() = default;

   void parse(const std::string& text);

   /**
    * \brief Get parameter value
    * \param name parameter name
    * \return std::optional<std::string>
    */
   std::optional<std::string> get(const std::string& name) const;

   /**
    * \brief Get parameter by position
    * \param position zero based index
    * \return std::optional<FielPtr>
    */
   std::optional<FieldPtr> get(int position) const;

};

using FormBodyPtr  = std::shared_ptr<FormBody>;
using FormBodyUPtr = std::unique_ptr<FormBody>;

/** @}*/

} // namespace http
} // namespace tbs