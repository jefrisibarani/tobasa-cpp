#pragma once

#include <asio.hpp>
#include <iostream>
#include <tobasa/non_copyable.h>
#include "tobasahttp/server/server_connection.h"
#include "tobasahttp/server/rate_limiter.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/**
 * Http server listener.
 * \tparam Traits
 */
template <class Traits>
class Listener : private NonCopyable
{
public:
   using Settings             = typename Traits::Settings;
   using Logger               = typename Traits::Logger;
   using ConnectionStarter    = typename Traits::ConnectionStarter;
   using Connection           = ServerConnection<Traits>;
   using ConnectionPtr        = std::shared_ptr<Connection>;
   using OnConnectionCreated  = std::function<void(ConnectionPtr)>;
   using AsioExecutor         = asio::any_io_executor;
   using Executor             = asio::strand<AsioExecutor>;

protected:
   asio::io_context&          _ioContext;
   asio::ip::tcp::acceptor    _acceptor;
   Settings&                  _settings;
   Logger&                    _logger;
   RequestHandler*            _pRequestHandler;
   ConnectionStarter          _starter;
   OnConnectionCreated        _onConnectionCreated;
   Executor                   _executor;
   RateLimiter                _rateLimiter;

public:

   Listener(
      asio::io_context& ioContext,
      Settings&         settings,
      Logger&           logger)
      : _ioContext { ioContext }
      , _executor  { _ioContext.get_executor() }
      , _acceptor  { _ioContext }
      , _settings  { settings }
      , _logger    { logger }
      , _starter   { settings, logger }
      , _rateLimiter(
          _settings.rateLimiterMaxRequests(),
          milliseconds(_settings.rateLimiterWindowDuration()),
          milliseconds(_settings.rateLimiterBlockDuration()),
          _settings.rateLimiterMaxViolations() )
   {}

   virtual ~Listener() = default;

   void start(RequestHandler* requestHandler, OnConnectionCreated handler)
   {
      if (_acceptor.is_open())
      {
         const auto ep = _acceptor.local_endpoint();
         _logger.error("[{}}] Server already started on {}", logHttpType(), toString(ep));
         return;
      }

      _pRequestHandler     = requestHandler;
      _onConnectionCreated = std::move(handler);

      //asio::ip::tcp::resolver resolver(_ioContext);
      //asio::ip::tcp::endpoint endpoint = *resolver.resolve(_settings.address(), _settings.port()).begin();
      asio::ip::tcp::endpoint endpoint( _settings.protocol(), _settings.port() );

      asio::error_code error;
      asio::ip::address address = asio::ip::make_address(_settings.address(), error);
      if (error)
      {
         _logger.debug("[{}] Error creating ip address: {}", logHttpType(), error.message());
         auto reason = tbsfmt::format("{} Server could not run on an incorrect ip address", logHttpType() );
         throw std::runtime_error(reason);
      }

      try
      {
         endpoint.address(address);
         // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
         _acceptor.open(endpoint.protocol());
         _acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
         _acceptor.bind(endpoint);
         _acceptor.listen();

         _logger.info("[{}] Server started on {}", logHttpType(), toString(endpoint));

         accept();
      }
      catch(const std::exception& ex)
      {
         if (_acceptor.is_open()) {
            _acceptor.close();
         }

         _logger.debug("[{}] Server error occured: {}", logHttpType(), ex.what());
         auto reason = tbsfmt::format("{} Server failed to start on {} {}", logHttpType(), toString(endpoint), ex.what());
         throw std::runtime_error(reason);
      }
   }

   void stop()
   {
      _acceptor.close();
   }

protected:

   /**
    * Get executor/strand, in single threaded, the type is asio::any_io_executor.
    * in multithreaded the type is asio::strand<asio::any_io_executor>
    */
   auto & executor() noexcept { return _executor; }

   void accept()
   {
      _acceptor.async_accept(
         asio::bind_executor(
            executor(),
            [this](std::error_code error, asio::ip::tcp::socket socket)
            {
               // Check whether the server was stopped by a signal before this
               // completion handler had a chance to run.
               if (!_acceptor.is_open()) {
                  return;
               }

               if (!error)
               {
                  // TODO_JEFRI: Remove logging
                  // Setup Socket send and receive buffer
                  asio::socket_base::send_buffer_size sndOpt;
                  asio::socket_base::receive_buffer_size rcvOpt;
                  socket.get_option(sndOpt);
                  socket.get_option(rcvOpt);
                  if (_settings.logVerbose())
                     _logger.trace("[{}] Current SO_SNDBUF: {} Bytes, SO_RCVBUF: {} Bytes ", logHttpType(), sndOpt.value(), rcvOpt.value() );
                  
                  asio::socket_base::send_buffer_size newSndOpt(_settings.sendBufferSize());
                  asio::socket_base::receive_buffer_size newRcvOpt(_settings.readBufferSize());
                  socket.set_option(newSndOpt);
                  socket.set_option(newRcvOpt);

                  socket.get_option(sndOpt);
                  socket.get_option(rcvOpt);
                  if (_settings.logVerbose())
                     _logger.trace("[{}] Updated SO_SNDBUF: {} Bytes, SO_RCVBUF: {} Bytes ", logHttpType(), sndOpt.value(), rcvOpt.value() );


                  if (_settings.useRateLimiter())
                  {
                     // Get the remote IP address
                     auto remoteIp = socket.remote_endpoint().address().to_string();
                     if (!_rateLimiter.allowRequest(remoteIp))
                        socket.close();
                     else
                        onAccepted(std::move(socket));
                  }
                  else
                     onAccepted(std::move(socket));
               }

               accept();

            } ) );
   }

   void onAccepted(asio::ip::tcp::socket socket)
   {
      // note compile error with GCC
      // expected ‘template’ keyword before dependent template name [-Wmissing-template-keyword]
      // we need to add template keyword
      _starter.template createConnection<Connection>(
         std::move(socket),
         *_pRequestHandler,
         [this](ConnectionPtr connection)
         {
            // hand over new connection to connection manager
            if (_onConnectionCreated) {
               _onConnectionCreated(std::move(connection));
            }
         });
   }

private:
private:
   std::string logHttpType() const
   {
      return logHttpTypeInfo(InstanceType::http_server, _settings.tlsMode());
   }
};

/** @}*/

} // namespace http
} // namespace tbs