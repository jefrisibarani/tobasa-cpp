#pragma once

#include <set>
#include <vector>
#include <string>
#include <asio.hpp>
#include <asio/io_context.hpp>
#include <tobasa/non_copyable.h>
#include "tobasahttp/connection_manager.h"
#include "tobasahttp/settings.h"
#include "tobasahttp/server/server_connection.h"
#include "tobasahttp/server/listener.h"
#include "tobasahttp/util.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/** 
 * HTTP Server
 * \tparam Traits
 */
template <class Traits>
class Server : private NonCopyable
{
   using Settings                  = typename Traits::Settings;
   using Logger                    = typename Traits::Logger;
   using Connection                = ServerConnection<Traits>;
   using ConnectionPtr             = std::shared_ptr<Connection>;
   using AsioExecutor              = asio::any_io_executor;
   using Executor                  = asio::strand<AsioExecutor>;
   using IoContextPtr              = std::shared_ptr<asio::io_context>;

private:
   IoContextPtr                    _pIoContext;
   asio::signal_set                _signals;
   Settings                        _settings;
   Logger&                         _logger;
   ConnectionMgr<Traits>           _connManager;
   Listener<Traits>                _listener;
   std::unique_ptr<RequestHandler> _pRequestHandler;
   bool                            _serverIsRunning;
   Executor                        _serverExecutor;
   StatusPageBuilder               _statusPageBuilder;
   bool                            _tlsMode;

   // Create a new shared pointer from an external 
   // instance of io_context and do not control its lifetime
   static IoContextPtr sharedFromExternal(asio::io_context& ctx)
   {
      return 
         std::shared_ptr<asio::io_context>(
            std::addressof(ctx),
            // Empty deleter.
            []( asio::io_context * ){} );
   }

public:

   asio::io_context& ioContext()
   {
      return *_pIoContext;
   }

   explicit Server(asio::io_context& ioContext, Settings settings, Logger& logger)
      : _pIoContext      { sharedFromExternal(ioContext) }
      , _serverExecutor  { (*_pIoContext).get_executor() }
      , _signals         { *_pIoContext }
      , _settings        { std::move(settings) }
      , _logger          { logger }
      , _connManager     { _settings, _logger, InstanceType::http_server }
      , _listener        { *_pIoContext, _settings, _logger }
      , _serverIsRunning { false }
   {
      _logger.trace("[{}] Server initialized", logHttpType());
      traceInfo();
      //setupSignals();
   }

   explicit Server(Settings settings, Logger& logger)
      : _pIoContext      { std::make_shared<asio::io_context>() }
      , _serverExecutor  { (*_pIoContext).get_executor() }
      , _signals         { *_pIoContext }
      , _settings        { std::move(settings) }
      , _logger          { logger }
      , _connManager     { _settings, _logger, InstanceType::http_server }
      , _listener        { *_pIoContext, _settings, _logger }
      , _serverIsRunning { false }
   {
      _logger.trace("[{}] Server initialized", logHttpType());
      traceInfo();
      //setupSignals();
   }
   

   ~Server()
   {
      stop();
      _logger.trace("[{}] Server destroyed", logHttpType());
   }

   void traceInfo()
   {
#ifdef TOBASA_HTTP_USE_HTTP2
  #ifdef TOBASA_HTTP2_WRITE_RESPONSE_NO_COPY_DATA
         _logger.trace("[{}] send buffer size: {}, read buffer size: {}, with nghttp2, nocopy_response_data", logHttpType(), _settings.sendBufferSize(), _settings.readBufferSize());
  #else
         _logger.trace("[{}] send buffer size: {}, read buffer size: {}, with nghttp2 ", logHttpType(), _settings.sendBufferSize(), _settings.readBufferSize());
  #endif
#else  
         _logger.trace("[{}] send buffer size: {}, read buffer size:{}", logHttpType(), _settings.sendBufferSize(), _settings.readBufferSize());
#endif
   }

   void requestHandler(std::unique_ptr<RequestHandler> handler)
   {
      _pRequestHandler = std::move(handler);
   }

   template<typename... Params>
   void requestHandler(Params &&... params) &
   {
      _pRequestHandler =
         std::make_unique< RequestHandler >(std::forward<Params>(params)...);
   }

   void statusPageBuilder(StatusPageBuilder builder)
   {
      _statusPageBuilder = std::move(builder);
   }

   /** 
    * Get executor/strand, in single threaded, the type is asio::any_io_executor.
    * in multithreaded the type is asio::strand<asio::any_io_executor>
    */
   auto & executor() noexcept { return _serverExecutor; }

   void start()
   {
      if (_serverIsRunning == false)
      {
         // start listener by passing request handler and OnConnectionCreated callback
         _listener.start(_pRequestHandler.get(),
            // OnConnectionCreated callback
            [&manager = _connManager, builder=_statusPageBuilder] (ConnectionPtr connection)
            {
               connection->statusPageBuilder(builder);
               manager.addConnection(std::move(connection));
            }
         );

         _serverIsRunning = true;
      }
   }

   void stop()
   {
      if (_serverIsRunning == true)
      {
         _listener.stop();
         _connManager.stopAll();

         _serverIsRunning = false;
      }
   }

   size_t totalConnections()
   {
      return _connManager.totalConnections();
   }
   
   ConnectionId lastConnectionId()
   {
      return _connManager.lastConnectionId();
   }
   
   std::vector<ConnectionInfo> currentConnectionsInfo()
   {
      return _connManager.currentConnectionsInfo();
   }

private:

   std::string logHttpType() const
   {
      return logHttpTypeInfo(InstanceType::http_server, _settings.tlsMode());
   }

#if 0
   void setupSignals()
   {
      // Register to handle the signals that indicate when the server should exit.
      // It is safe to register for the same signal multiple times in a program,
      // provided all registration for the specified signal is made through Asio.
      _signals.add(SIGINT);
      _signals.add(SIGTERM);
#if defined(SIGQUIT)
      _signals.add(SIGQUIT);
#endif // defined(SIGQUIT)

      waitStopSignals();
   }

   void waitStopSignals()
   {
      _signals.async_wait(
         [this](std::error_code /*ec*/, int /*signo*/)
         {
            stop();
         });
   }
#endif // #if 0

};

/** @}*/

} // namespace http
} // namespace tbs