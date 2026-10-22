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
      try
      {
         if (_factory && _factory->getPlugin() != nullptr)
            unloadPlugin();
      }
      catch (const std::exception& e)
      {
         Logger::logE("Exception while unloading plugin from: " + _path + ". " + e.what());
      }
      catch (...)
      {
         Logger::logE("Unknown exception while unloading plugin from: " + _path);
      }

      try
      {
         _lib->unload();
      }
      catch (const std::exception& e)
      {
         Logger::logE("Exception while unloading library: " + _path + ". " + e.what());
      }
      catch (...)
      {
         Logger::logE("Unknown exception while unloading library: " + _path);
      }

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

   try
   {
      _factory->createPlugin();
   }
   catch (const std::exception& e)
   {
      Logger::logE("Exception creating plugin from: " + _path + ". " + e.what());
      return false;
   }
   catch (...)
   {
      Logger::logE("Unknown exception creating plugin from: " + _path);
      return false;
   }

   Plugin* plugin = _factory->getPlugin();
   if (plugin)
   {
      _plugin = plugin;
      try
      {
         plugin->load();
         return true;
      }
      catch (const std::exception& e)
      {
         Logger::logE("Exception loading plugin from: " + _path + ". " + e.what());
         try
         {
            _factory->deletePlugin();
         }
         catch (const std::exception& ex)
         {
            Logger::logE("Exception deleting failed plugin from: " + _path + ". " + ex.what());
         }
         _plugin = nullptr;
         return false;
      }
      catch (...)
      {
         Logger::logE("Unknown exception loading plugin from: " + _path);
         try
         {
            _factory->deletePlugin();
         }
         catch (const std::exception& ex)
         {
            Logger::logE("Exception deleting failed plugin from: " + _path + ". " + ex.what());
         }
         _plugin = nullptr;
         return false;
      }
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
      try
      {
         plugin->unload();
      }
      catch (const std::exception& e)
      {
         Logger::logE("Exception unloading plugin from: " + _path + ". " + e.what());
      }
      catch (...)
      {
         Logger::logE("Unknown exception unloading plugin from: " + _path);
      }

      try
      {
         _factory->deletePlugin();
      }
      catch (const std::exception& e)
      {
         Logger::logE("Exception deleting plugin from: " + _path + ". " + e.what());
      }
      catch (...)
      {
         Logger::logE("Unknown exception deleting plugin from: " + _path);
      }

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
   if (_factory != nullptr)
   {
      const char* name = _factory->name();
      return name ? std::string(name) : std::string();
   }

   return "";
}


std::string PluginLibrary::getPluginId() const
{
   if (_factory != nullptr)
   {
      const char* id = _factory->id();
      return id ? std::string(id) : std::string();
   }

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