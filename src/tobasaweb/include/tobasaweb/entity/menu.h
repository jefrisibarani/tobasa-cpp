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

struct Menu
{
   long        id;
   std::string name;
   long        groupId;
   long        typeId;
   std::string pageurl;
   bool        sidebar;
   std::string methods;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Menu, id, name, groupId, typeId, pageurl, sidebar, methods)

using MenuPtr  = std::shared_ptr<Menu>;
using MenuList = std::vector<Menu>;


struct MenuView
{
   long        id;
   std::string name;
   std::string menuType;
   std::string menuGroup;
   std::string pageurl;
   bool        sidebar;
   std::string methods;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MenuView, id, name, menuType, menuGroup, pageurl, sidebar, methods)
using MenuViewPtr  = std::shared_ptr<MenuView>;
using MenuViewList = std::vector<MenuView>;

struct MenuGroupView
{
   long        id;
   std::string name;
   std::string code;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MenuGroupView, id, name, code)
using MenuGroupViewPtr  = std::shared_ptr<MenuGroupView>;
using MenuGroupViewList = std::vector<MenuGroupView>;

struct MenuTypeView
{
   long        id;
   std::string name;
   std::string code;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MenuTypeView, id, name, code)
using MenuTypeViewPtr  = std::shared_ptr<MenuTypeView>;
using MenuTypeViewList = std::vector<MenuTypeView>;

/** @}*/

} // namespace entity
} // namespace web
} // namespace tbs