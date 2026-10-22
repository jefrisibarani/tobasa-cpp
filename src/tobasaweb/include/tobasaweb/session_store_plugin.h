#pragma once

#include <memory>
#include <string>

#include <tobasa/json.h>
#include <tobasa/plugin.h>

namespace tbs {
namespace web {

class ISessionStore;

struct SessionStoreConfig
{
   std::string appDataDir;
};


class SessionStorePlugin : public Plugin
{
public:
   SessionStorePlugin() = default;
   virtual ~SessionStorePlugin() override = default;

   virtual void load() = 0;
   virtual void unload() = 0;

   virtual std::unique_ptr<ISessionStore> createStore(const SessionStoreConfig& config) const = 0;
};


} // namespace web
} // namespace tbs
