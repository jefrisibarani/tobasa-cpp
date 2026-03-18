#pragma once

#include <mutex>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace tbs {

/**
 * @ingroup TBS
 * PluginManager
 */

class Plugin;

class PluginLibrary;

class PluginManager
{
public:
   static PluginManager& get();

   PluginManager(const PluginManager&) = delete;
   PluginManager& operator=(const PluginManager&) = delete;

   bool loadLibrary(const std::string& path);
   bool loadPlugin(const std::string& path);
   void unloadPlugin(const std::string& name);
   void unloadAllPlugins();

   Plugin* getPlugin(const std::string& name) const;
   bool hasPlugin(const std::string& name) const;

   // Autoload management (basic support)
   void setPluginAutoLoad(const std::string& name, bool autoLoad);
   bool isPluginAutoLoad(const std::string& name) const;

   std::vector<std::string> getPluginNames() const;
private:
   PluginManager() = default;
   ~PluginManager();

   PluginLibrary* findPluginLibrary(const std::string& name) const;

   std::vector<std::unique_ptr<PluginLibrary>> _pluginLibs;
   std::unordered_map<std::string, bool> _autoload;
   mutable std::mutex _mutex;
};

} // namespace tbs
