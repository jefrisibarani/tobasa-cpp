#pragma once

#include <tobasahttp/server/common.h>
#include <tobasaweb/result.h>

namespace tbs {
namespace web {

namespace dom {

struct Menu
{
   Menu() {}
   Menu(const std::string& menuCaption, const std::string& menuRequestPath, const std::string& menuicon)
      : caption(menuCaption), requestPath(menuRequestPath), icon(menuicon) {}
      
   std::string caption;
   std::string requestPath;
   std::string icon;
   std::string name;
   std::vector<Menu> menuList;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Menu, caption, requestPath, icon, name, menuList)

struct MenuGroup
{
   std::string groupName;
   std::string caption;
   std::string icon;
   std::vector<Menu> menuList;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MenuGroup, groupName, caption, icon, menuList)

using MenuGroupList = std::vector<MenuGroup>;

struct DatatableOpt
{
   bool useAutoFill     = false;
   bool useButtons      = false;
   bool useDateTime     = false;
   bool useFixedColumns = false;
   bool useFixedHeader  = false;
   bool useKeyTable     = false;
   bool useResponsive   = false;
   bool useRowGroup     = false;
   bool useRowReorder   = false;
   bool useRowDelete    = false;
   bool useJSZip        = false;
   bool usePdfMake      = false;
   bool useRowEdit      = false;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DatatableOpt, useAutoFill, useButtons, useDateTime, useFixedColumns,
   useFixedHeader, useKeyTable, useResponsive, useRowGroup, useRowReorder, useRowDelete, useJSZip, usePdfMake, useRowEdit)

} // namespace dom


/**
 * View
 *
 */
class View
{
public:
   View();
   virtual ~View();
   
   /// Render template from file, can be stored in disk or memory
   /// @param tpl template file name
   /// @param inMemoryContext the context to find template when using in memory resorces
   virtual std::string render(const std::string& tpl, const std::string& inMemoryContext="");
   virtual void setData(const Json& data);

protected:
   std::string _template;
   Json _data;
   
   // the context to find template when using in memory resorces
   std::string _inMemoryContext;
};


/**
 * Page
 *
 */
class Page : public View
{
protected:
   http::HttpContext _httpcontext;

public:
   Page(http::HttpContext contex);
   virtual ~Page();

   Page& setTemplate(const std::string& tpl);
   Page& title(const std::string& title);

   template<typename T>
   Page& data(const std::string& name, const T& value)
   {
      _data[name] = value;
      return *this;
   }

   Json& data()
   {
      return _data;
   }

   /// Render/Show template from file, can be stored in disk or memory
   /// @param tpl template file name
   /// @param inMemoryContext the context to find template when using in memory resorces
   http::ResultPtr show(const std::string& tpl, const std::string& inMemoryContext="");

   static void releaseMode(bool value=true);
   static void homePage(const std::string& homePage);

   static web::dom::MenuGroupList menuGroupList;

   static std::string appBuildMode;
   static std::string appHomePage;
};

} // namespace web
} // namespace tbs