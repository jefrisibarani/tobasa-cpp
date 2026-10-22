#include <algorithm>
#include <filesystem>

#include "tobasa/plugin_manager.h"
#include "tobasa/logger.h"
#include "tobasa/path.h"
#include "tobasa/plugin_lib.h"
#include "tobasa/plugin.h"
#include "tobasa/util_string.h"

namespace tbs {


PluginManager& PluginManager::get()
{
   static PluginManager instance;
   return instance;
}

PluginManager::~PluginManager()
{
   unloadAllPlugins();
}

bool PluginManager::loadLibrary(const std::string& path)
{
   std::lock_guard<std::mutex> lock(_mutex);

   if (findLibraryByPath(path))
      return true;

   auto lib = std::make_unique<PluginLibrary>(path);
   if (!lib->load())
      return false;

   _pluginLibs.push_back(std::move(lib));
   return true;
}

bool PluginManager::loadPlugin(const std::string& path)
{
   std::lock_guard<std::mutex> lock(_mutex);

   auto* existing = findLibraryByPath(path);
   if (existing)
   {
      if (existing->hasPlugin())
         return true;

      return existing->loadPlugin();
   }

   auto lib = std::make_unique<PluginLibrary>(path);
   if (!lib->load() || !lib->loadPlugin())
      return false;

   _pluginLibs.push_back(std::move(lib));
   return true;
}



void PluginManager::loadPluginsFromFolder(std::vector<std::string>& folders)
{
   namespace fs = std::filesystem;

   for (const auto& dir : folders)
   {
      fs::path dirPath(dir);

      if (!fs::exists(dirPath) || !fs::is_directory(dirPath))
         continue;

      for (const auto& entry : fs::directory_iterator(dirPath, fs::directory_options::skip_permission_denied))
      {
         const auto status = entry.symlink_status();
         if (!fs::is_regular_file(status))
            continue;

         loadPlugin(entry.path().string());
      }
   }
}

void PluginManager::unloadPlugin(const std::string& name)
{
   std::lock_guard<std::mutex> lock(_mutex);

   auto it = std::remove_if(_pluginLibs.begin(), _pluginLibs.end(),
      [&](const std::unique_ptr<PluginLibrary>& lib) {
         if (lib->getName() == name)
         {
            lib->unloadPlugin();
            lib->unload();
            return true;
         }
         return false;
      });

   _pluginLibs.erase(it, _pluginLibs.end());
}

void PluginManager::unloadAllPlugins()
{
   std::lock_guard<std::mutex> lock(_mutex);

   for (auto& lib : _pluginLibs)
   {
      lib->unloadPlugin();
      lib->unload();
   }
   _pluginLibs.clear();
}

Plugin* PluginManager::getPlugin(const std::string& name) const
{
   std::lock_guard<std::mutex> lock(_mutex);

   auto lib = findLibraryByName(name);
   return lib ? lib->getPlugin() : nullptr;
}

Plugin* PluginManager::getPluginById(const std::string& id) const
{
   std::lock_guard<std::mutex> lock(_mutex);

   auto lib = findLibraryByPluginId(id);
   return lib ? lib->getPlugin() : nullptr;
}

bool PluginManager::hasPlugin(const std::string& name) const
{
   std::lock_guard<std::mutex> lock(_mutex);
   return findLibraryByName(name) != nullptr;
}

void PluginManager::setPluginAutoLoad(const std::string& name, bool autoLoad)
{
   std::lock_guard<std::mutex> lock(_mutex);
   _autoload[name] = autoLoad;
}

bool PluginManager::isPluginAutoLoad(const std::string& name) const
{
   std::lock_guard<std::mutex> lock(_mutex);

   auto it = _autoload.find(name);
   return it != _autoload.end() && it->second;
}

std::vector<std::string> PluginManager::getPluginNames() const
{
   std::lock_guard<std::mutex> lock(_mutex);

   std::vector<std::string> names;
   names.reserve(_pluginLibs.size());

   for (const auto& lib : _pluginLibs)
   {
      if (lib)
         names.push_back(lib->getName());
   }

   return names;
}

PluginLibrary* PluginManager::findLibraryByPath(const std::string& path) const
{
   if (path.empty())
      return nullptr;

   const auto requestedStem = std::filesystem::path(path).stem().string();

   const auto sameNormalizedPath = [](const std::string& lhs, const std::string& rhs) {
      auto left = path::normalize(lhs);
      auto right = path::normalize(rhs);
#ifdef _WIN32
      util::strLower(left);
      util::strLower(right);
#endif
      return left == right;
   };

   for (const auto& lib : _pluginLibs)
   {
      if (!lib || lib->getPath().empty())
         continue;

      if (sameNormalizedPath(lib->getPath(), path) ||
          lib->getName() == requestedStem)
      {
         return lib.get();
      }
   }

   return nullptr;
}

PluginLibrary* PluginManager::findLibraryByName(const std::string& name) const
{
   for (auto& lib : _pluginLibs)
   {
      if (!lib)
         continue;

      if (lib->getName() == name)
         return lib.get();
   }
   return nullptr;
}


PluginLibrary* PluginManager::findLibraryByPluginName(const std::string& name) const
{
   for (auto& lib : _pluginLibs)
   {
      if (!lib)
         continue;

      if (lib->getPluginName() == name)
         return lib.get();
   }
   return nullptr;
}

PluginLibrary* PluginManager::findLibraryByPluginId(const std::string& id) const
{
   for (auto& lib : _pluginLibs)
   {
      if (!lib)
         continue;

      const auto pluginId = lib->getPluginId();
      if (pluginId == id)
         return lib.get();
   }
   return nullptr;
}

} // namespace tbs
