#pragma once

#include <tobasa/non_copyable.h>
#include "tobasahttp/server/common.h"
#include "tobasahttp/server/settings.h"
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
class ConnectionStarter : private NonCopyable
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

   ConnectionStarter(Settings& settings, Logger& logger)
      : _settings     { settings }
      , _logger       { logger }
      , _instanceType { InstanceType::http_server }
   {
      _logger.trace("[{}] ConnectionStarter initialized", logHttpType());
   }

   ~ConnectionStarter()
   {
      _logger.trace("[{}] ConnectionStarter destroyed", logHttpType());
   }

   /**
    * Create connection.
    * \tparam ConnectionType
    * \param socket
    * \param requestHandler
    * \param onCreateHandler
    */
   template<class ConnectionType>
   void createConnection(
      asio::ip::tcp::socket socket,
      RequestHandler&       requestHandler,
      std::function<void(std::shared_ptr<ConnectionType>)> onCreatedHandler)
   {
      _logger.info("[{}] Starting HTTP connection with {}", logHttpType(), toString(socket.lowest_layer().remote_endpoint()));

      auto conn = std::make_shared<ConnectionType>(
         std::move(socket), _settings, _logger, requestHandler);

      // setup connection's onStart callback. this callback will be called from connection's start()
      // which is called from connection manager.
      conn->onStart =
         [](Socket& sock, std::function<void(const std::error_code&)> onStartCallback )
         {
            onStartCallback(std::error_code());
         };
      
      // Server Connection hast just been created, call the handler
      // for example to hand over connection to onCreatedHandler, 
      // to be processed further by connection manager
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