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
   ResponseHandler*           _pResponseHandler;
   ConnectionStarter          _starter;
   Executor                   _executor;

   OnConnectionCreated        _onConnectionCreated;
   OnConnectFailed            _onConnectFailed;

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
      _onConnectionCreated = nullptr;
      _onConnectFailed = nullptr;

      _logger.trace("[http_client] Connector destroyed");
   }

   void start(
      ResponseHandler  responseHandler,
      OnConnectionCreated createdHandler,
      OnConnectFailed  failConnectHandler)
   {
      _onConnectionCreated = std::move(createdHandler);
      _onConnectFailed     = std::move(failConnectHandler);

      _resolver.async_resolve( _settings.protocol(), _settings.address(),
         std::to_string(_settings.port()),
         [this, responseHandler=std::move(responseHandler)]
         (const asio::error_code& error, const asio::ip::tcp::resolver::results_type& endPointList)
         {
            if (!error)
               connect(responseHandler, endPointList);
            else
            {
               auto message = error.message();
               _logger.error("[http_client] {}, address is: {}", message, _settings.address());

               if (_onConnectFailed)
                  _onConnectFailed(message);
            }
         });
   }

   void stop()
   {
   }

   void onConnectFailed(OnConnectFailed handler)
   {
      _onConnectFailed = std::move(handler);
   }

protected:

   /** 
    * Get executor/strand, in single threaded, the type is asio::any_io_executor.
    * In multithreaded the type is asio::strand<asio::any_io_executor>
    */
   auto & executor() noexcept { return _executor; }

   void connect(ResponseHandler responseHandler,
      const asio::ip::tcp::resolver::results_type& endPointList)
   {
      asio::ip::tcp::socket socket(_ioContext);
      // note compile error with GCC
      // expected ‘template’ keyword before dependent template name [-Wmissing-template-keyword]
      // we need to add template keyword
      _starter.template createClientSession<Connection>( std::move(socket), std::move(responseHandler),
         [this, &endPointList](ConnectionPtr connection)
         {
            doConnect(std::move(connection), endPointList);
         });
   }

   void doConnect(ConnectionPtr connection,
      const asio::ip::tcp::resolver::results_type& endPointList)
   {
      // Start the asynchronous connect operation.
      asio::async_connect( connection->socket().lowest_layer(), endPointList,
         // connect condition
         [this](const asio::error_code& error, const asio::ip::tcp::endpoint& next)
         {
            auto message = error.message();
            if (error)
               _logger.error("[http_client] Connector error, ", message);

            _logger.info("[http_client] Trying connect to {}", toString(next));
            
            return true;
         },
         //  completion token/ completion handler
         [this,connection](const std::error_code& error, const asio::ip::tcp::endpoint& ep)
         {
            if (!error)
            {
               _logger.info("[http_client] Connected to {}", toString(ep));
               // hand over new connection to connection manager
               if (_onConnectionCreated)
                  _onConnectionCreated(std::move(connection));
            }
            else
            {
               auto message = error.message();
               _logger.error("[http_client] {}", message);

               if (_onConnectFailed)
                  _onConnectFailed(message);
            }
         }
      );
   }

};

/** @}*/

} // namespace http
} // namespace tbs