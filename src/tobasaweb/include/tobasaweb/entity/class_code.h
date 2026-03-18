#pragma once

#include <string>
#include <memory>

namespace tbs {
namespace web {
namespace entity {

/** \ingroup WEB
 * @{
 */

struct ClassCode
{
   int         id;
   std::string itemName;
   std::string itemClass;
   int         itemId;
   std::string itemCode;
};

using ClassCodePtr = std::shared_ptr<ClassCode>;

/** @}*/

} // namespace entity
} // namespace web
} // namespace tbs