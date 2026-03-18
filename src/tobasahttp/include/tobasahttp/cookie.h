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
 * \brief Http Request Cookie.
 * \sa https://httpwg.org/specs/rfc6265.html#cookie
 * Cookie: name=value; name=value; name=value
 */
class Cookie
   : public FieldCollection
{
public:
   Cookie() = default;
   ~Cookie() = default;
   
   /// Parse cookies from raw HTTP cookies test
   void parse(const std::string& text);

   /// Get cookies from Http headers collection
   void fromCollection(Headers& headers);
};

using CookiePtr = std::shared_ptr<Cookie>;

/** @}*/

} // namespace http
} // namespace tbs