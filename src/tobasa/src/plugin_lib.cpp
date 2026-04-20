#include <vector>
#include <filesystem>
#include "tobasa/logger.h"
#include "tobasa/dynamic_lib.h"
#include "tobasa/plugin.h"
#include "tobasa/plugin_lib.h"

namespace tbs {

PluginLibrary::PluginLibrary(const std::string& path)
   : _path(path)
{
   _lib = std::make_unique<DynamicLib>();
   _name = std::filesystem::path(path).stem().string();  
}

PluginLibrary::~PluginLibrary()
{
   unload();
}

bool PluginLibrary::load()
{
   if (_loaded)
      return true;

   if (!_lib->load(_path))
   {
      Logger::logE("Failed to load library: " + _path);
      return false;
   }

   auto fn = reinterpret_cast<GetTobasaPluginFn>(_lib->getSymbol(GETTOBASAPLUGIN));
   if (!fn)
   {
      Logger::logE("Factory export not found in: " + _path);
      _lib->unload();
      return false;
   }

   _factory = fn();
   if (!_factory)
   {
      Logger::logE("Factory creation failed in: " + _path);
      _lib->unload();
      return false;
   }

   _loaded = true;
   return true;
}

void PluginLibrary::unload()
{
   if (_loaded)
   {
      if (_factory && _factory->getPlugin() != nullptr)
         unloadPlugin();

      _lib->unload();
      _factory = nullptr;
      _plugin = nullptr;
      _loaded = false;
   }
}

bool PluginLibrary::loadPlugin()
{
   if (!_loaded)
   {
      if (!load())
         return false;
   }

   if (!_factory)
      return false;

   _factory->createPlugin();
   Plugin* plugin = _factory->getPlugin();
   if (plugin)
   {
      _plugin = plugin;
      plugin->load();
      return true;
   }
   else
   {
      Logger::logE("Failed to create plugin from: " + _path);
      return false;
   }
}

void PluginLibrary::unloadPlugin()
{
   if (!_factory)
      return;

   Plugin* plugin = _factory->getPlugin();
   if (plugin != nullptr)
   {
      plugin->unload();
      _factory->deletePlugin();
      _plugin = nullptr;
   }
}

bool PluginLibrary::isLoaded() const
{
   return _loaded;
}

bool PluginLibrary::hasPlugin() const
{
   return ( _factory && _factory->getPlugin() ) ;
}

std::string PluginLibrary::getPluginName() const
{
   if (_factory != nullptr )
      return _factory->name();
   
   return "";
}

Plugin* PluginLibrary::getPlugin() const
{
   if (_factory && _factory->getPlugin())
      return _factory->getPlugin();
   else 
      return nullptr;
}


} // namespace tbs