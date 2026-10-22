#pragma once

#include <string>
#include <tobasa/plugin.h>

namespace tbs {

/**
 * @brief Simple dummy plugin for demonstration purposes
 * 
 * This plugin demonstrates the basic plugin architecture by
 * implementing the required Plugin interface methods.
 */
class DummyPlugin : public Plugin
{
public:
   DummyPlugin() = default;
   ~DummyPlugin() override = default;

   /**
    * @brief Called when the plugin is loaded
    */
   void load() override;

   /**
    * @brief Called when the plugin is unloaded
    */
   void unload() override;

   /**
    * @brief Get plugin information
    */
   std::string getInfo() const;

private:
   std::string _info {"DummyPlugin is loaded"};
};

} // namespace tbs