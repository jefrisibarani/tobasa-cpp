#include <tobasa/exp_imp.h>
#include <tobasa/logger.h>
#include <iostream>
#include "hello.h"

namespace tbs {

/**
 * @brief Factory for HelloPlugin
 * 
 * Creates and manages HelloPlugin instances. This factory is retrieved
 * by the PluginManager when the plugin library is loaded.
 */
static PluginFactory<HelloPlugin> helloPluginFactory(
   "Hello Plugin",           // Plugin name
   "hello_plugin",           // Plugin unique ID
   "Tobasa Team",            // Author
   "1.0.0",                  // Version
   "A simple hello plugin for testing", // Description
   PluginType::addon         // Plugin type
);

/**
 * @brief Export function for plugin loading
 * 
 * This function must be exported from the plugin DLL/SO and is called
 * by PluginLibrary to retrieve the plugin factory.
 */
TBS_PLUGIN_API IPluginFactory* GetTobasaPlugin()
{
   return &helloPluginFactory;
}


void HelloPlugin::load()
{
   Logger::logI("HelloPlugin: load() called");
   std::cout << "HelloPlugin loaded successfully!" << std::endl;
}

void HelloPlugin::unload()
{
   Logger::logI("HelloPlugin: unload() called");
   std::cout << "HelloPlugin unloaded successfully!" << std::endl;
}

std::string HelloPlugin::getInfo() const
{
   return _info;
}


} // namespace tbs
