#pragma once

#include <string>
#include <memory>

namespace tbs {
namespace web {
namespace entity {

/** \ingroup WEB
 * @{
 */

struct UserSite
{
   long id;
   long userId;
   long siteId;
   bool allowLogin;
   bool isSysAdmin;
};

using UserSitePtr = std::shared_ptr<UserSite>;


struct UserSiteView
{
   long        id;
   long        userId;
   std::string userName;
   int         siteId;
   std::string siteName;
};

using UserSiteViewPtr = std::shared_ptr<UserSiteView>;

/** @}*/

} // namespace entity
} // namespace web
} // namespace tbs