#pragma once

#include <string>
#include <memory>

namespace tbs {

class IPluginFactory;
class Plugin;
class DynamicLib;

class PluginLibrary
{
public:
   explicit PluginLibrary(const std::string& path);
   ~PluginLibrary();

   PluginLibrary(const PluginLibrary&) = delete;
   PluginLibrary& operator=(const PluginLibrary&) = delete;

   PluginLibrary(PluginLibrary&&) noexcept = default;
   PluginLibrary& operator=(PluginLibrary&&) noexcept = default;

   bool load();
   void unload();

   bool loadPlugin();
   void unloadPlugin();

   bool isLoaded() const;
   bool hasPlugin() const;
   std::string getName() const { return _name; } ;
   std::string getPluginName() const;

   Plugin* getPlugin() const;

private:
   std::string      _name;
   bool             _pluginLoaded;
   bool             _pluginEnabled;
   std::string      _path;

   std::unique_ptr<DynamicLib> _lib;

   Plugin* _plugin {nullptr};
   IPluginFactory* _factory {nullptr};
   bool _loaded{false};

   friend class PluginManager;
};

} // namespace tbs   