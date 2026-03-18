#include <iostream>
#include <sstream>  // for normalizePath
#include <vector>   // for normalizePath
#include <filesystem>

#include <tobasa/util_string.h>
#include <tobasa/logger.h>
#include "app_resource.h"

#include "resources/config_resources.h"
#include "resources/tls_asset_resources.h"

#ifdef TOBASA_BUILD_IN_MEMORY_RESOURCES
   #include "resources/template_resources.h"
   #include "resources/wwwroot_resources.h"
   #include "resources/template_lis_resources.h"
#endif

namespace tbs {
namespace app {

// Function to normalize a path
// Used For resolving path inside in-memory file system
// valid path delimiter is '/'
std::string normalizePath(const std::string& path)
{
   const char pathDelimiter = '/';
   bool isAbsolute = !path.empty() && path.front() == '/';

   std::stringstream ss(path);
   std::string token;
   std::vector<std::string> components;

   // Split into components
   while (std::getline(ss, token, pathDelimiter)) 
   {
      // If ".." is encountered, remove the last component (go up one folder level)
      if (token == "..") 
      {
         if (!components.empty())
            components.pop_back();
      }
      // Ignore "." and empty components 
      else if (token != "." && !token.empty()) 
      {
         components.push_back(token);
      }
   }

   // Reconstruct
   std::string normalizedPath;
   for (size_t i = 0; i < components.size(); ++i) 
   {
      normalizedPath += components[i];
      if (i < components.size() - 1)
         normalizedPath += pathDelimiter;
   }

   if (isAbsolute)
      normalizedPath.insert(normalizedPath.begin(), '/');

   if (normalizedPath.empty() && isAbsolute)
      normalizedPath = "/";

   return normalizedPath;
}

Resource::Object::Object(const std::string& path, const std::string& content)
   : path {path}, content {content} {}

Resource::Object::~Object() {}

nonstd::span<const unsigned char> Resource::get(const std::string& path, const std::string& searchContext)
{
   std::string context = searchContext;
   auto newPath = normalizePath(path);

#ifdef TOBASA_BUILD_IN_MEMORY_RESOURCES
   // when opening a template with context other than appview,
   // and the template extends a layout
   // Force to search in appview to resolve the layout
   if (util::startsWith(newPath,"views/layouts/") && searchContext != "appview" ) {
      context = "appview";
   }
#endif

   // Application configuration files
   if (context == "config")
      return res::getConfigResources(newPath);

   // TLS Assets
   if (context == "tls_asset")
      return res::getTlsAssetResources(newPath);

#ifdef TOBASA_BUILD_IN_MEMORY_RESOURCES
   // appview and wwwroot : main application search context
   if (context == "appview")
      return res::getTemplateResources(newPath);
   if (context == "wwwroot")
      return res::getWwwrootResources(newPath);


 #if defined(TOBASA_USE_LIS_ENGINE)
   if (context == "appview_lis")
      return res::getTemplateLisResources(newPath);
 #endif //defined(TOBASA_USE_LIS_ENGINE)

   if (!util::endsWith(path,".map"))
      Logger::logW("[webapp] Resource {} is empty", newPath );

#endif //TOBASA_BUILD_IN_MEMORY_RESOURCES

   return {};
}

std::string Resource::getString(const std::string& path, const std::string& searchContext)
{
   auto data = Resource::get(path, searchContext);
   if (data.empty())
      return "";

   return std::string( reinterpret_cast<const char*>(data.data()), data.size() );
}

bool Resource::exists(const std::string& path, const std::string& searchContext )
{
   return  ! get(path, searchContext ).empty();
}

void Resource::add(const std::string& path, const std::string& content)
{}

} // namespace app
} // namespace tbs