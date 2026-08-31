#pragma once

#include <asio.hpp>
#include <tobasa/non_copyable.h>
#include "tobasahttp/client/client_connection.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/** 
 * Http client connector
 * \tparam Traits
 */
template <class Traits>
class Connector : private NonCopyable
{
public:
   using Settings             = typename Traits::Settings;
   using Logger               = typename Traits::Logger;
   using ConnectionStarter    = typename Traits::ConnectionStarter;
   using Connection           = ClientConnection<Traits>;
   using ConnectionPtr        = std::shared_ptr<Connection>;
   using OnConnectionCreated  = std::function<void(ConnectionPtr)>;
   using AsioExecutor         = asio::any_io_executor;
   using Executor             = asio::strand<AsioExecutor>;

protected:
   asio::io_context&          _ioContext;
   asio::ip::tcp::resolver    _resolver;
   Settings&                  _settings;
   Logger&                    _logger;
   ConnectionStarter          _starter;
   Executor                   _executor;
   bool                       _stopped = false;

public:

   Connector(
      asio::io_context& ioContext,
      Settings&         settings,
      Logger&           logger)
      : _ioContext { ioContext }
      , _executor  { _ioContext.get_executor() }
      , _resolver  { ioContext }
      , _settings  { settings }
      , _logger    { logger }
      , _starter   { settings, logger}
   {
      _logger.trace("[http_client] Connector initialized");
   }

   virtual ~Connector()
   {
      _logger.trace("[http_client] Connector destroyed");
   }

   void start(
      ClientResponseHandler responseHandler,
      OnConnectionCreated createdHandler,
      OnConnectFailed failedHandler)
   {
      _stopped = false;

      _resolver.async_resolve( 
         _settings.protocol(), 
         _settings.address(),
         std::to_string(_settings.port()),
         [this,
         responseHandler = std::move(responseHandler),
         createdHandler = std::move(createdHandler),
         failedHandler = std::move(failedHandler)]
         (const asio::error_code& error, asio::ip::tcp::resolver::results_type endPointList) mutable
         {
            if (_stopped)
               return;

            if (error)
            {
               auto message = error.message();
               _logger.error("[http_client] Resolve failed: {}, address is: {}", message, _settings.address());

               if (error != asio::error::operation_aborted && failedHandler) {
                  failedHandler(error.message());
               }

               return;
            }

            connect(
               std::move(responseHandler),
               std::move(createdHandler),
               std::move(failedHandler),
               std::move(endPointList));
         });
   }

   void stop()
   {
      _stopped = true;
      _resolver.cancel();
   }

protected:

   /** 
    * Get executor/strand, in single threaded, the type is asio::any_io_executor.
    * In multithreaded the type is asio::strand<asio::any_io_executor>
    */
   auto & executor() noexcept { return _executor; }

   void connect(
      ClientResponseHandler responseHandler,
      OnConnectionCreated createdHandler,
      OnConnectFailed failedHandler,
      asio::ip::tcp::resolver::results_type endPointList)
   {
      asio::ip::tcp::socket socket(_ioContext);
      // note compile error with GCC
      // expected ‘template’ keyword before dependent template name [-Wmissing-template-keyword]
      // we need to add template keyword
      _starter.template createClientSession<Connection>( 
         std::move(socket), 
         std::move(responseHandler),
         [this,
         createdHandler = std::move(createdHandler),
         failedHandler = std::move(failedHandler),
         endPointList = std::move(endPointList)]  (ConnectionPtr connection) mutable
         {
            doConnect(
               std::move(connection),
               endPointList,
               std::move(createdHandler),
               std::move(failedHandler));
         });
   }

   void doConnect(
      ConnectionPtr connection,
      const asio::ip::tcp::resolver::results_type& endPointList,
      OnConnectionCreated createdHandler,
      OnConnectFailed failedHandler)
   {
      // Start the asynchronous connect operation.
      asio::async_connect(
         connection->socket().lowest_layer(), 
         endPointList,
         // connect condition
         [this](const asio::error_code& error, const asio::ip::tcp::endpoint& endpoint)
         {
            if (_stopped)
               return false;

            if (error)
               _logger.error("[http_client] Connector error, ", error.message());

            _logger.info("[http_client] Trying connect to {}", toString(endpoint));

            return true;
         },
         //  completion token/ completion handler
         [this,
          connection,
          createdHandler = std::move(createdHandler),
          failedHandler = std::move(failedHandler)]
         (const asio::error_code& error, const asio::ip::tcp::endpoint& endpoint) mutable
         {
            if (_stopped)
               return;

            if (!error)
            {
               _logger.info("[http_client] Connected to {}", toString(endpoint));
               // hand over new connection to connection manager
               if (createdHandler)
                  createdHandler(std::move(connection));
            }
            else
            {
               _logger.error("[http_client] Connection failed: {}", error.message());

               if (error != asio::error::operation_aborted && failedHandler) {
                  failedHandler(error.message());
               }
            }
         }
      );
   }

};

/** @}*/

} // namespace http
} // namespace tbs