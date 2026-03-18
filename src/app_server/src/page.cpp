#include <tobasa/config.h>
#include <tobasa/logger.h>
#include <tobasa/util.h>
#include <tobasa/uuid.h>
#include <tobasahttp/mimetypes.h>
#include <tobasahttp/request.h>
#include <tobasaweb/settings_webapp.h>
#include <tobasaweb/credential_info.h>
#include <tobasaweb/json_result.h>
#include <tobasaweb/alert.h>
#include <tobasaweb/session.h>

#include "app_resource.h"
#include "app_common.h"
#include "inja_util.h"
#include "page.h"

namespace tbs {
namespace web {

View::View()
  : _template("")
  , _inMemoryContext("appview")
{
   _data["pageTitle"]     = "Tobasa Web Service";
   _data["pageBaseUrl"]   = "/";
   _data["pageBodyClass"] = "bg-primary";
   _data["pageAlerts"]    = "[]";
}

View::~View() {}

void View::setData(const Json& data)
{
   // merge data
   if (!data.empty())
      _data.update(data, true);
}

std::string View::render(const std::string& tpl, const std::string& inMemoryContext)
{
   if (! inMemoryContext.empty())
      _inMemoryContext = inMemoryContext;

   _template = tpl;
   if (_template.compare(0, 2, "./") == 0) 
      _template.erase(0, 2);

   try
   {
      std::string output;

      auto templateDir = app::templateDir() + path::SEPARATOR;

      inja::Environment env(templateDir);
      env.add_callback("intToAlphaChar", 1, [](inja::Arguments& args) {
         int number = args.at(0)->get<int>(); 
         return injautil::intToAlphaChar(number);
      });

      env.add_callback("randomId", 1, [](inja::Arguments& args) {
         int total = args.at(0)->get<int>(); 
         return uuid::generate();
      });

      env.add_callback("lookupArray", 3, [](inja::Arguments& args) {
         return injautil::lookupArray(args);
      });      

      env.add_callback("comboBox", 6, [](inja::Arguments& args) {
         return injautil::comboBox(args);
      });     

      env.add_callback("appResourceUrl", 2, 
      [this](inja::Arguments& args) {
         std::string baseUrl = _data["pageBaseUrl"].get<std::string>();
         return injautil::appResourceUrl(args, baseUrl );
      });     
            
      env.add_callback("gender", 1, 
      [this](inja::Arguments& args) {
         return injautil::gender(args);
      });

      env.add_callback("currentYear", 0,
      [this](inja::Arguments& args) {
         return DateTime::now().format("{:%Y}");
      });

      env.add_callback("currentDate", 0,
      [this](inja::Arguments& args) {
         return DateTime::now().format("{:%Y-%m-%d}");
      });

      env.add_callback("currentTime", 0,
      [this](inja::Arguments& args) {
         return DateTime::now().format("{:%H:%M:%S}");
      });

#ifdef TOBASA_BUILD_IN_MEMORY_RESOURCES
      if (conf::Webapp::useInMemoryResources)
      {
         env.set_search_included_templates_in_files(false);

         // The callback takes the current path and the wanted include name and returns a template
         env.set_include_callback(
            [this,&env](const std::string& path, const std::string& name)
            {
               std::string templateName = name;
               std::string originalPath = static_cast<std::string>(path);
               std::string originalName = templateName;

               // Build the relative path
               templateName = originalPath + originalName;
               if (templateName.compare(0, 2, "./") == 0)
                  templateName.erase(0, 2);

               auto parser = env.create_parser();
               auto tpl = inja::Template(app::Resource::getString("views/" + templateName, _inMemoryContext ));
               parser.parse_into_template(tpl, templateName);
               return tpl;
            });

         auto parser = env.create_parser();
         auto tpl = inja::Template( app::Resource::getString("views/" + _template, _inMemoryContext) );
         parser.parse_into_template(tpl, _template);
         output = env.render(tpl, _data);
      }
      else {
         output = env.render_file(_template, _data);
      }
#else
      output = env.render_file(_template, _data);
#endif

      return output;
   }
   catch(const std::exception& ex )
   {
      Logger::logE("[webapp] View::render, could not render view template: {}", ex.what() );
      throw std::runtime_error("Could not render view template");
   }
}

web::dom::MenuGroupList Page::menuGroupList = {};
std::string Page::appBuildMode = "DEVELOPMENT";
std::string Page::appHomePage = "/";

void Page::releaseMode(bool value)
{
   if (value)
      appBuildMode = "PRODUCTION";
   else
      appBuildMode = "DEVELOPMENT";
}

void Page::homePage(const std::string& homePage)
{
   appHomePage = homePage;
}

Page::Page(http::HttpContext contex)
   : _httpcontext(contex)
{
   auto session = Session::get(_httpcontext->sessionId());

   _data["appBuildMode"] = Page::appBuildMode;
   _data["appHomePage"]  = Page::appHomePage;
   _data["appMainTitle"] = "Tobasa Web Service";
   _data["pageLang"]     = "en";
   _data["pageBaseUrl"]  = _httpcontext->request()->baseUrl();

   // is session not loades, means session file does not exist, indicated this might be request after loggedout, 
   // which already deleted session file. 
   // we don't want to create a new session file
   if ( ! session->loaded() )
      _data["pageAlerts"] = Json::array();
   else
      _data["pageAlerts"] = session->getAlerts();
}

Page::~Page()
{
   Logger::logT("[webapp] [conn:{}] page destroyed", _httpcontext->connId());
}

Page& Page::setTemplate(const std::string& tpl)
{
   _template = tpl;
   return *this;
}

Page& Page::title(const std::string& title)
{
   _data["pageTitle"] = title;
   return *this;
}

http::ResultPtr Page::show(const std::string& tpl, const std::string& inMemoryContext)
{
   try
   {
      if ( Page::menuGroupList.size() > 0 ) {
         _data["sidebarMenu"] = Page::menuGroupList;
      }

      auto content = render(tpl, inMemoryContext);
      auto result = http::makeResult();
      result->contentType("text/html");
      result->content(std::move(content));

      return std::move(result);
   }
   catch(const std::exception& ex )
   {
      Logger::logE("[webapp] [conn:{}] Page::show, {}", _httpcontext->connId(), ex.what());
      return http::statusResultHtml(http::StatusCode::INTERNAL_SERVER_ERROR);
   }
}

} // namespace web
} // namespace tbs