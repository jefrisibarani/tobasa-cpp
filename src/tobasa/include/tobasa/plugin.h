#pragma once

#include <string>
#include "tobasa/exp_imp.h"

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
class TBS_API Plugin
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
class TBS_API IPluginFactory
{
public:
   IPluginFactory() = default;
   virtual ~IPluginFactory() = default;

   virtual PluginType  type() const        = 0;
   virtual const char* name() const        = 0;
   virtual const char* id() const          = 0;
   virtual const char* author() const      = 0;
   virtual const char* version() const     = 0;
   virtual const char* description() const = 0;

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
   PluginFactory(const std::string& name,
                 const std::string& id,
                 const std::string& author,
                 const std::string& version,
                 const std::string& description = "",
                 PluginType type = PluginType::addon)
      : _type(type),
        _name(name),
        _id(id),
        _author(author),
        _version(version),
        _description(description)
   {}

   ~PluginFactory() override { deletePlugin(); }

   PluginType  type() const override         { return _type; }
   const char* name() const override         { return _name.c_str(); }
   const char* id() const override           { return _id.c_str(); }
   const char* author() const override       { return _author.c_str(); }
   const char* version() const override      { return _version.c_str(); }
   const char* description() const override  { return _description.c_str(); }

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
   Plugin*     _pPlugin {nullptr};
   PluginType  _type;
   std::string _name;
   std::string _id;
   std::string _author;
   std::string _version;
   std::string _description;
};

/// Exported symbol name for factory retrieval
inline constexpr const char* GETTOBASAPLUGIN = "GetTobasaPlugin";

/// Function pointer type for exported factory getter
using GetTobasaPluginFn = IPluginFactory* (*)();

/** @}*/

} // namespace tbs