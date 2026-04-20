#include <tobasa/exp_imp.h>
#include <tobasa/logger.h>
#include <iostream>
#include "dummy.h"

namespace tbs {

/**
 * @brief Factory for DummyPlugin
 * 
 * Creates and manages DummyPlugin instances. This factory is retrieved
 * by the PluginManager when the plugin library is loaded.
 */
static PluginFactory<DummyPlugin> dummyPluginFactory(
   "Dummy Plugin",           // Plugin name
   "dummy_plugin",           // Plugin unique ID
   "Tobasa Team",            // Author
   "1.0.0",                  // Version
   "A simple dummy plugin for testing", // Description
   PluginType::addon         // Plugin type
);

/**
 * @brief Export function for plugin loading
 * 
 * This function must be exported from the plugin DLL/SO and is called
 * by PluginLibrary to retrieve the plugin factory.
 */
TBS_PLUGIN IPluginFactory* GetTobasaPlugin()
{
   return &dummyPluginFactory;
}


void DummyPlugin::load()
{
   Logger::logI("DummyPlugin: load() called");
   std::cout << "DummyPlugin loaded successfully!" << std::endl;
}

void DummyPlugin::unload()
{
   Logger::logI("DummyPlugin: unload() called");
   std::cout << "DummyPlugin unloaded successfully!" << std::endl;
}

std::string DummyPlugin::getInfo() const
{
   return _info;
}


} // namespace tbs
