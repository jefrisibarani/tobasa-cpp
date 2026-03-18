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
   unloadPlugin();
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

   _factory = reinterpret_cast<IPluginFactory*>(_lib->getSymbol(GETTOBASAPLUGIN));
   if (!_factory)
   {
      Logger::logE("Factory not found in: " + _path);
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
      if (_plugin != nullptr)
         unloadPlugin();

      _lib->unload();
      _factory = nullptr;
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
   if (_factory->getPlugin() != nullptr)
   {
      _factory->getPlugin()->unload();
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