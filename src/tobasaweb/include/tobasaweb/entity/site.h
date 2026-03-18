#pragma once

#include <string>
#include <memory>

namespace tbs {
namespace web {
namespace entity {

/** \ingroup WEB
 * @{
 */

struct Site
{
   long id;
   std::string code;
   std::string name;
   std::string address;
};

using SitePtr = std::shared_ptr<Site>;

/** @}*/

} // namespace entity
} // namespace web
} // namespace tbs