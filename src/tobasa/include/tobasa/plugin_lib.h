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
   std::string      _path;
   bool             _loaded  {false};
   bool             _enabled {true};
   Plugin*          _plugin  {nullptr};
   IPluginFactory*  _factory {nullptr};
   std::unique_ptr<DynamicLib> _lib;

   friend class PluginManager;
};

} // namespace tbs   