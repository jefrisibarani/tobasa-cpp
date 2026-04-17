#pragma once

#include <tobasa/non_copyable.h>
#include "tobasahttp/client/response.h"
#include "tobasahttp/client/settings.h"
#include "tobasahttp/util.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/** 
 * Connection starter for http.
 * \tparam SettingsType
 * \tparam LoggerType
 */
template<class SettingsType, class LoggerType>
class ClientConnectionStarter : private NonCopyable
{
public:
   using Settings = SettingsType;
   using Logger   = LoggerType;
   using Socket   = asio::ip::tcp::socket;

private:
   Settings&      _settings;
   Logger&        _logger;
   InstanceType   _instanceType;

protected:
   void init() {}

public:

   ClientConnectionStarter(Settings& settings, Logger& logger)
      : _settings     { settings }
      , _logger       { logger }
      , _instanceType { InstanceType::http_client }
   {
      _settings.tlsMode(false);
      _logger.trace("[{}] ClientConnectionStarter initialized", logHttpType());
   }

   ~ClientConnectionStarter()
   {
      _logger.trace("[{}] ClientConnectionStarter destroyed", logHttpType());
   }


   /**
    * Create client session
    * \tparam ConnectionType
    * \param socket
    * \param responseHandler
    * \param onCreateHandler
    */
   template<class ConnectionType>
   void createClientSession(
      asio::ip::tcp::socket&& socket,
      ClientResponseHandler   responseHandler,
      std::function<void(std::shared_ptr<ConnectionType>)> onCreatedHandler)
   {
      auto conn = std::make_shared<ConnectionType>(
         std::move(socket), _settings, _logger, responseHandler);

      // setup connection's onStart callback. this callback will be called from connection's start()
      // which is called from connection manager.
      conn->onStart =
         [](Socket& sock, std::function<void(const std::error_code&)> onStartCallback )
         {
            onStartCallback(std::error_code());
         };

      // hand over connection to onCreatedHandler, to be processed further by connection manager
      onCreatedHandler(std::move(conn));
   }

   std::string logHttpType() const
   {
      return logHttpTypeInfo(_instanceType, _settings.tlsMode());
   }
};

/** @}*/

} // namespace http
} // namespace tbs