#pragma once

#include <string>
#include <memory>

namespace tbs {

class IPluginFactory;
class Plugin;
class DynamicLib;

/**
 * @ingroup TBS
 * Represents a dynamically loaded Tobasa plugin library and its plugin instance.
 */
class PluginLibrary
{
public:
   /**
    * Creates a plugin library for the specified shared library path.
    * @param path Path to the plugin library.
    */
   explicit PluginLibrary(const std::string& path);

   /// Destroys the library and unloads any loaded plugin.
   ~PluginLibrary();

   /// Plugin libraries cannot be copied.
   PluginLibrary(const PluginLibrary&) = delete;

   /// Plugin libraries cannot be copy-assigned.
   PluginLibrary& operator=(const PluginLibrary&) = delete;

   /// Moves a plugin library and its state into a new object.
   PluginLibrary(PluginLibrary&&) noexcept = default;

   /// Moves a plugin library and its state into an existing object.
   PluginLibrary& operator=(PluginLibrary&&) noexcept = default;

   /// Loads the shared library and resolves its plugin factory.
   bool load();

   /// Unloads the plugin and shared library, if loaded.
   void unload();

   /// Creates and loads the plugin from the resolved factory.
   bool loadPlugin();

   /// Unloads and destroys the plugin instance, if present.
   void unloadPlugin();

   /// Returns whether the shared library is loaded.
   bool isLoaded() const;

   /// Returns whether the library currently has a plugin instance.
   bool hasPlugin() const;

   /// Returns the library name derived from its path.
   std::string getName() const { return _name; } ;

   /// Returns the name reported by the plugin factory.
   std::string getPluginName() const;

   /// Returns the current plugin instance, or nullptr if none exists.
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