#include <tobasa/path.h>
#include <tobasa/config.h>
#include <tobasaweb/settings_webapp.h>
#include "app_common.h"

namespace tbs {
namespace app {

namespace fs = std::filesystem;

std::string getDirFromConfig(const std::string& name)
{
   auto option = Config::getOption<web::conf::Webapp>("webapp");
   std::string res;
   
   if (name == "data")
      res = option.webService.dataDir;
   else if (name == "upload")
      res = option.webService.uploadDir;

   if (res.empty())
      return res;

   fs::path p = res;
   return p.make_preferred().string();
}

std::string getAppDir(const std::string& name)
{  
   auto getAppDataDir = []() {
      auto res = getDirFromConfig("data");
      if (res.empty())
         res = path::appdataDir() + path::SEPARATOR + app::APP_NAME;

      return res;
   };

   static std::string appDataDir = getAppDataDir();
   std::string res;

   if (name == "data") {
      res = appDataDir;
   }
   else if (name == "images") {
      res = appDataDir + path::SEPARATOR + "images";
   }
   else if (name == "report") {
      res = appDataDir + path::SEPARATOR + "report";
   }
   else if (name == "upload")
   {
      res = getDirFromConfig("upload");
      if (res.empty())
         res = appDataDir + path::SEPARATOR + "upload";
   }
   else if (name == "session") {
      res = appDataDir + path::SEPARATOR + "session";
   }   

   if (!res.empty() && !path::exists(res)) {
      path::createDir(res);
   }

   return res;
}

std::string dataDir()
{
   return getAppDir("data");
}

std::string configDir()
{
   static std::string confDir = path::executableDir() + path::SEPARATOR + "configuration";
   return confDir;
}

std::string imageDir()
{
   static std::string imgDir = getAppDir("images");
   return imgDir;
}

std::string reportDir()
{
   static std::string confDir = getAppDir("report");
   return confDir;
}

std::string uploadDir()
{
   static std::string uploadDir = getAppDir("upload");
   return uploadDir;
}

std::string sessionDir()
{
   static std::string sesDir = getAppDir("session");
   return sesDir;
}


// -------------------------------------------------------

std::string getDocRoot()
{
   auto option = Config::getOption<web::conf::Webapp>("webapp");
   return path::resolveExecutableRelativePath(option.httpServer.docRoot);
}

std::string getTemplateDir()
{
   auto option = Config::getOption<web::conf::Webapp>("webapp");
   return path::resolveExecutableRelativePath(option.webService.templateDir);
}

std::string docRoot()
{
   static std::string docRoot = getDocRoot();
   return docRoot;
}

std::string templateDir()
{
   static std::string templateDir = getTemplateDir();
   return templateDir;
}

std::string currentWorkingDir()
{
   return fs::current_path().string();
}

}} // namespace tbs::app