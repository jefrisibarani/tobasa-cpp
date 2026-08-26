#pragma once

#include <mutex>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace tbs {

class Plugin;

class PluginLibrary;

/**
 * @ingroup TBS 
 * Manages dynamically loaded plugin libraries and plugin instances.
 */
class PluginManager
{
public:
   /// Returns the singleton plugin manager instance.
   static PluginManager& get();

   /// Plugin managers cannot be copied.
   PluginManager(const PluginManager&) = delete;

   /// Plugin managers cannot be copy-assigned.
   PluginManager& operator=(const PluginManager&) = delete;

   /**
    * Loads and registers a plugin library without creating its plugin.
    * @param path Path to the plugin library.
    * @return True if the library was loaded successfully.
    */
   bool loadLibrary(const std::string& path);

   /**
    * Loads a plugin library and creates its plugin instance.
    * @param path Path to the plugin library.
    * @return True if the library and plugin were loaded successfully.
    */
   bool loadPlugin(const std::string& path);

   /// Unloads and removes the plugin library with the specified name.
   void unloadPlugin(const std::string& name);

   /// Unloads and removes all registered plugin libraries.
   void unloadAllPlugins();

   /// Returns the plugin with the specified name, or nullptr if it is not found.
   Plugin* getPlugin(const std::string& name) const;

   /// Returns whether a plugin library with the specified name is registered.
   bool hasPlugin(const std::string& name) const;

   /// Sets the autoload preference for the specified plugin name.
   void setPluginAutoLoad(const std::string& name, bool autoLoad);

   /// Returns whether autoload is enabled for the specified plugin name.
   bool isPluginAutoLoad(const std::string& name) const;

   /// Returns the names of all registered plugin libraries.
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
