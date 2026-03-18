#include "tobasa/plugin_manager.h"
#include "tobasa/logger.h"
#include "tobasa/path.h"
#include "tobasa/plugin_lib.h"
#include "tobasa/plugin.h"

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

   auto lib = std::make_unique<PluginLibrary>(path);
   if (!lib->load())
      return false;

   _pluginLibs.push_back(std::move(lib));
   return true;
}

bool PluginManager::loadPlugin(const std::string& path)
{
   std::lock_guard<std::mutex> lock(_mutex);

   auto lib = std::make_unique<PluginLibrary>(path);
   if (!lib->load() || !lib->loadPlugin())
      return false;

   _pluginLibs.push_back(std::move(lib));
   return true;
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

   auto lib = findPluginLibrary(name);
   return lib ? lib->getPlugin() : nullptr;
}

bool PluginManager::hasPlugin(const std::string& name) const
{
   std::lock_guard<std::mutex> lock(_mutex);
   return findPluginLibrary(name) != nullptr;
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

PluginLibrary* PluginManager::findPluginLibrary(const std::string& name) const
{
   for (auto& lib : _pluginLibs)
   {
      if (lib->getName() == name)
         return lib.get();
   }
   return nullptr;
}


} // namespace tbs
