#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <tobasa/path.h>
#include <tobasa/plugin_manager.h>

namespace {

std::string pluginFileName(const std::string& baseName)
{
#ifdef _WIN32
   return baseName + ".dll";
#elif defined(__APPLE__)
   return baseName + ".dylib";
#else
   return baseName + ".so";
#endif
}

std::filesystem::path resolvePluginDirectory()
{
   const char* envValue = std::getenv("SAMPLE_APP_PLUGIN_FOLDER");
   if (envValue && *envValue)
   {
      std::filesystem::path configured(envValue);
      if (std::filesystem::exists(configured))
         return configured;
   }

   const auto executableRelative = tbs::path::resolveExecutableRelativePath("plugins");
   if (!executableRelative.empty())
   {
      const std::filesystem::path relativeDir(executableRelative);
      if (std::filesystem::exists(relativeDir))
         return relativeDir;
   }

   const std::filesystem::path fallback = std::filesystem::path(tbs::path::executableDir()) / "plugins";
   std::filesystem::create_directories(fallback);
   return fallback;
}

void printPluginStatus(const tbs::PluginManager& manager, const std::map<std::string, std::string>& loadedPluginPaths)
{
   const auto names = manager.getPluginNames();
   std::cout << "\nLoaded plugin list:\n";
   if (names.empty())
   {
      std::cout << "  none\n";
      return;
   }

   for (const auto& name : names)
   {
      const auto it = loadedPluginPaths.find(name);
      const std::string path = it != loadedPluginPaths.end() ? it->second : "(path unavailable)";
      std::cout << "  - " << name << " -> " << path << '\n';
   }
}

} // namespace

int main()
{
   try
   {
      const std::filesystem::path pluginDirectory = resolvePluginDirectory();
      std::cout << "Plugin directory: " << pluginDirectory << '\n';

      const std::vector<std::pair<std::string, std::string>> plugins = {
         {"plugin_dummy", (pluginDirectory / pluginFileName("plugin_dummy")).string()},
         {"plugin_hello", (pluginDirectory / pluginFileName("plugin_hello")).string()}
      };

      tbs::PluginManager& manager = tbs::PluginManager::get();
      std::map<std::string, std::string> loadedPluginPaths;

      for (const auto& [pluginName, pluginPath] : plugins)
      {
         std::cout << "Loading " << pluginName << " from " << pluginPath << "...\n";
         if (!std::filesystem::exists(pluginPath))
         {
            std::cerr << "  [missing] plugin library not found\n";
            continue;
         }

         const bool loaded = manager.loadPlugin(pluginPath);
         std::cout << (loaded ? "  [ok]" : "  [failed]") << " plugin load result\n";

         if (loaded)
         {
            loadedPluginPaths[pluginName] = pluginPath;
            auto* plugin = manager.getPlugin(pluginName);
            if (plugin)
               std::cout << "  plugin object created: " << pluginName << '\n';
         }
      }

      printPluginStatus(manager, loadedPluginPaths);

      std::cout << "\nUnloading plugin_dummy on demand...\n";
      manager.unloadPlugin("plugin_dummy");
      printPluginStatus(manager, loadedPluginPaths);

      const auto dummyPath = plugins.front().second;
      std::cout << "\nReloading plugin_dummy from " << dummyPath << "...\n";
      if (manager.loadPlugin(dummyPath))
         std::cout << "  [ok] reloaded\n";
      else
         std::cout << "  [failed] reload\n";

      printPluginStatus(manager, loadedPluginPaths);

      std::cout << "\nUnloading all plugins...\n";
      manager.unloadAllPlugins();
      printPluginStatus(manager, loadedPluginPaths);

      return 0;
   }
   catch (const std::exception& ex)
   {
      std::cerr << "Exception: " << ex.what() << '\n';
      return 1;
   }
}
