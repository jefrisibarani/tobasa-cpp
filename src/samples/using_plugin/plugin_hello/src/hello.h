#pragma once

#include <string>
#include <tobasa/plugin.h>

namespace tbs {

/**
 * @brief Simple plugin for demonstration purposes
 * 
 * This plugin demonstrates the basic plugin architecture by
 * implementing the required Plugin interface methods.
 */
class HelloPlugin : public Plugin
{
public:
   HelloPlugin() = default;
   ~HelloPlugin() override = default;

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
   std::string _info {"HelloPlugin is loaded"};
};

} // namespace tbs