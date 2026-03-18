#pragma once

#include <string>
#include <memory>
#include <tobasa/json.h>

namespace tbs {
namespace web {
namespace entity {

/** \ingroup WEB
 * @{
 */

struct Acl
{
   long        id;
   long        ugId;
   std::string ugType;
   long        menuId;
   bool        aAll;
   bool        aAdd;
   bool        aDelete;
   bool        aUpdate;
   bool        aPrint;
   bool        aIndex;
   std::string aOther;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Acl, id, ugId, ugType, menuId, aAll, aAdd, aDelete, aUpdate, aPrint, aIndex, aOther)

using AclPtr  = std::shared_ptr<Acl>;
using AclList = std::vector<Acl>;

struct AclView
{
   int         id;
   int         ugId;
   std::string ugType;
   std::string ugName;
   int         menuId;
   std::string menuName;
   std::string menuGroup;
   std::string menuType;
   std::string pageUrl;
   bool        aAll;
   bool        aIndex;
   bool        aAdd;
   bool        aDelete;
   bool        aUpdate;
   bool        aPrint;
   std::string aOther;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AclView, id, ugId, ugType, ugName, menuId, menuName,
   menuGroup, menuType, pageUrl, aAll, aAdd, aDelete, aUpdate, aPrint, aIndex, aOther)

using AclViewPtr  = std::shared_ptr<AclView>;
using AclViewList = std::vector<AclView>;

/** @}*/

} // namespace entity
} // namespace web
} // namespace tbs