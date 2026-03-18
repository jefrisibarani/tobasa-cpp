#pragma once

#include <string>

namespace tbs {

/** @addtogroup TBS
 * @{
 */

class IPluginFactory;

enum class PluginType
{
   module,
   addon
};

/**
 * @brief Base class for plugins
 * @note Managed by its factory
 */
class Plugin
{
public:
   Plugin() = default;
   virtual ~Plugin() = default;
   virtual void load() = 0;
   virtual void unload() = 0;
   const IPluginFactory* factory() const noexcept { return _pFactory; }
   void setFactory(const IPluginFactory* factory) noexcept { _pFactory = factory; }

private:
   const IPluginFactory* _pFactory{nullptr};
   friend class IPluginFactory;
};

/**
 * @brief Factory interface for plugins
 */
class IPluginFactory
{
public:
   IPluginFactory() = default;
   virtual ~IPluginFactory() = default;

   virtual PluginType type() const = 0;
   virtual std::string name() const = 0;
   virtual std::string id() const = 0;
   virtual std::string author() const = 0;
   virtual std::string version() const = 0;
   virtual std::string description() const = 0;

protected:
   virtual Plugin* getPlugin() = 0;
   virtual void createPlugin() = 0;
   virtual void deletePlugin() = 0;

private:
   friend class PluginLibrary;

   IPluginFactory(const IPluginFactory&) = default;
   IPluginFactory& operator=(const IPluginFactory&) = default;
};


/**
 * @brief Generic plugin factory implementation
 */
template <class P>
class PluginFactory : public IPluginFactory
{
public:
   PluginFactory(std::string name,
                 std::string id,
                 std::string author,
                 std::string version,
                 std::string desc = "",
                 PluginType type = PluginType::addon)
      : _pluginType(type),
        _pluginName(std::move(name)),
        _pluginId(std::move(id)),
        _pluginAuthor(std::move(author)),
        _pluginVersion(std::move(version)),
        _pluginDesc(std::move(desc))
   {}

   ~PluginFactory() override { deletePlugin(); }

   PluginType type() const override { return _pluginType; }
   std::string name() const override { return _pluginName; }
   std::string id() const override { return _pluginId; }
   std::string author() const override { return _pluginAuthor; }
   std::string version() const override { return _pluginVersion; }
   std::string description() const override { return _pluginDesc; }

protected:

   Plugin* getPlugin() override { return _pPlugin; }

   void createPlugin() override
   {
      if (!_pPlugin)
      {
         _pPlugin = new P();
         _pPlugin->setFactory(this);
      }
   }

   void deletePlugin() override
   {
      delete _pPlugin;
      _pPlugin = nullptr;
   }

private:
   Plugin* _pPlugin{nullptr};
   PluginType _pluginType;
   std::string _pluginName;
   std::string _pluginId;
   std::string _pluginAuthor;
   std::string _pluginVersion;
   std::string _pluginDesc;
};

/// Exported symbol name for factory retrieval
inline constexpr const char* GETTOBASAPLUGIN = "GetTobasaPlugin";

/// Function pointer type for exported factory getter
using GetTobasaPluginFn = IPluginFactory* (*)();

/** @}*/

} // namespace tbs