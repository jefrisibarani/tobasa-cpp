#pragma once

#include <set>
#include <vector>
#include <string>
#include <deque>
#include <asio/io_context.hpp>
#include <tobasa/non_copyable.h>
#include "tobasahttp/headers.h"
#include "tobasahttp/connection_manager.h"
#include "tobasahttp/client/response.h"
#include "tobasahttp/client/request.h"
#include "tobasahttp/client/settings.h"
#include "tobasahttp/client/connector.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */


/** 
  HTTP Client
  \tparam Traits
  
   http::Client is a simple HTTP client class.
   It is used to send HTTP requests to a server and receive responses.
   This client supports two modes:
   1. Synchronous mode (blocking)
      - Do not call run()
      - Requests are executed one by one

   2. Asynchronous mode (non-blocking)
      - Must call run()
      - Supports multiple concurrent connections

   Do not mix both modes in the same Client instance.

   Async mode Example:
   \code
      using namespace tbs::http;

      http::SettingsClient settings;
      settings
         .address("127.0.0.1")
         .port(8085)
         .tlsMode(true)
         .logVerbose(true)
         .connectionPoolSize(4)        // create max 4 connections
         .maxRequestsPerConnection(0); // disable

      http::SecureClientDefault client(std::move(settings), logger);

      // With a maximum of 4 connections, in async mode, 100 requests will
      // create 4 connections, each handling about 25 requests.

      // In async mode, shutdown() must be called,
      // otherwise the client will keep running.
      // It is safe to shutdown when no connections remain.
      // This can be checked in onConnectFailed() and onConnectionClosed() handlers.
      auto checkShutdown = [&]() {
         auto nConn = client.totalConnections();
         if( nConn == 0 )
         {
            logger.info("Shutting down Client");
            client.shutdown();
         }
      };

      // Setup handlers
      // ----------------------------------

      client.onConnectionError( [&](const http::ErrorData& error) {
         logger.error(error.message);
      });

      // check if we need to shutdown and log error
      client.onConnectFailed( [&](const std::string& message) {
         logger.error(message);
         checkShutdown();
      });

      // check if we need to shutdown
      client.onConnectionClosed([&](http::ConnectionId connId) {
         checkShutdown();
      });

      auto processResponse = [&](const http::ClientResponsePtr& response)
      {
         for ( size_t i = 0; i < response->headers().size(); i++)
         {
            auto f = response->headers().field(i);
            if ( f != nullptr )
               std::cout << "Http header     : " <<  f->name() << " : " << f->value() << std::endl;
         }
         std::cout << response->content() << std::endl;
      };

      // add default header for connection
      client.addHeader("User-Agent", "Tobasa Client");

      // Note: Do not mix async and sync request. use only one mode
      bool useSync = true;
      if (useSync)
      {
         // run 100 Sync requests
         for (int i=0;i<100;i++)
         {
            auto res = client.get( "/api/version");
            if (res)
               processResponse(res);
         }
      }
      else
      {
         // run 100 Async request
         std::string resource = "/api/version";
         for (int i=0;i<100;i++)
         {
            client.get("/api/version", [&](const http::ClientResponsePtr& response) {
               processResponse(response);
            });
         }

         // in async request, client will run until shutdown() called
         client.run();
      }
   \endcode


 */
template <class Traits>
class Client : private NonCopyable
{
private:   
   using Settings            = typename Traits::Settings;
   using Logger              = typename Traits::Logger;
   using Connection          = ClientConnection<Traits>;
   using ConnectionPtr       = std::shared_ptr<Connection>;
   using AsioExecutor        = asio::any_io_executor;
   using Executor            = asio::strand<AsioExecutor>;
   using IoContextPtr        = std::shared_ptr<asio::io_context>;
   using WorkGuard           = asio::executor_work_guard<AsioExecutor>;

private:
   IoContextPtr              _pIoContext;
   Settings                  _settings;
   Logger&                   _logger;
   ConnectionMgr<Traits>     _connManager;
   Connector<Traits>         _connector;
   Executor                  _clientExecutor;
   std::unique_ptr<WorkGuard> _workGuard;

   OnConnectFailed           _onConnectFailed;
   OnConnectionTimeOut       _onConnectionTimeOut;
   OnConnectionError         _onConnectionError;
   OnConnectionClosed        _onConnectionClosed;
   
   bool                      _internalIoCtx;

   std::deque<ConnectionPtr> _idleConnections;
   size_t                    _maxConnections    = 2;
   size_t                    _activeConnections = 0;

   Headers                   _defaultHeaders;

   std::deque< std::pair<ClientRequest, ClientResponseHandler> > _pendingRequests;

   enum class Mode {
      None,
      Sync,
      Async
   };

   Mode _mode = Mode::None;

private:
   asio::io_context& ioContext()
   {
      return *_pIoContext;
   }

   ConnectionPtr acquireConnection()
   {
      while (!_idleConnections.empty())
      {
         auto conn = _idleConnections.front();
         _idleConnections.pop_front();

         if (conn->closed())
            continue;

         if (conn->busy())
            continue;

         return conn;
      }

      return nullptr;
   }

   void releaseConnection(ConnectionPtr conn)
   {
      if (!_pendingRequests.empty())
      {
         auto [req, handler] = std::move(_pendingRequests.front());
         _pendingRequests.pop_front();

         sendRequest(conn, std::move(req));

         return;
      }

      if (_idleConnections.size() < _maxConnections)
      {
         _logger.info("[{}] releasing connection", logHttpType());
         _idleConnections.push_back(std::move(conn));
      }
   }

   std::unique_ptr<Request> buildRequest(ClientRequest req)
   {
      auto httpReq = std::make_unique<Request>(HttpVersion::one);

      std::string host = tbsfmt::format("{}:{}", _settings.address(), _settings.port());
      httpReq->addHeader("Host", host);
      httpReq->addHeader("Connection", "keep-alive");
      
      httpReq->method(req.method());
      httpReq->target(req.target());

      if (!req.content().empty())
      {
         httpReq->content(req.content());
      }

      for (auto h : _defaultHeaders.getValues() ) {
         httpReq->addHeader(h->name(), h->value());
      }

      for (auto h: req.headers().getValues()) {
         httpReq->addHeader(h->name(), h->value());
      }

      return httpReq;
   }

   void sendRequest(ConnectionPtr conn, ClientRequest req)
   {
      auto request = buildRequest(std::move(req));
      conn->markBusy();
      conn->request(std::move(request));
      conn->start();
   }

   void ensureMode(Mode requested)
   {
      if (_mode == Mode::None)
      {
         _mode = requested;
         return;
      }

      if (_mode != requested)
      {
         throw std::logic_error("Client: cannot mix synchronous and asynchronous requests");
      }
   }

public:

   explicit Client(Settings settings, Logger& logger)
      : _pIoContext     { std::make_shared<asio::io_context>() }
      , _clientExecutor { (*_pIoContext).get_executor() }
      , _settings       { std::move(settings) }
      , _logger         { logger }
      , _connManager    { _settings, _logger, InstanceType::http_client }
      , _connector      { *_pIoContext, _settings, _logger }
      , _internalIoCtx  { true }
   {
      _logger.trace("[{}] Client initialized", logHttpType());

      _connManager.onConnectionTimeOut([this](const TimeOutData& data) {
         handleConnectionTimeOut(data);
      });
      
      _connManager.onConnectionError([this](const http::ErrorData& error) {
         handleConnectionError(error);
      });

      _connManager.onConnectionClosed([this](ConnectionId connId) {
         handleConnectionClosed(connId);
      });

      _maxConnections = _settings.connectionPoolSize();
   }

   void shutdown()
   {
      stop();

      if (_internalIoCtx && _pIoContext)
      {
         if (!_pIoContext->stopped())
            _pIoContext->stop();
      }

      _mode = Mode::None;

      _onConnectFailed     = nullptr;
      _onConnectionTimeOut = nullptr;
      _onConnectionError   = nullptr;
      _onConnectionClosed  = nullptr;
   }

   virtual ~Client()
   {
      shutdown();
      _logger.trace("[{}] Client destroyed", logHttpType());
   }

   void onConnectFailed(OnConnectFailed handler)         { _onConnectFailed     = std::move(handler); }
   void onConnectionTimeOut(OnTimeOut handler)           { _onConnectionTimeOut = std::move(handler); }
   void onConnectionError(OnConnectionError handler)     { _onConnectionError   = std::move(handler); }   
   void onConnectionClosed(OnConnectionClosed handler)   { _onConnectionClosed  = std::move(handler); }   


   /// Get executor/strand, in single threaded, the type is asio::any_io_executor.
   /// in multithreaded the type is asio::strand<asio::any_io_executor>
   auto & executor() noexcept { return _clientExecutor; }

   void addHeader(const std::string& key,const std::string& value)
   {
      _defaultHeaders.add(key, value);
   }

   // ---------------------------------------
   // Asynchronous requests
   // ---------------------------------------

   /// Async GET
   void get(const std::string& resource, ClientResponseHandler handler)
   {
      ClientRequest r("GET", resource);
      execute(std::move(r), std::move(handler));
   }

   /// Async DEL
   void del(const std::string& resource, ClientResponseHandler handler)
   {
      ClientRequest r("DEL", resource);
      execute(std::move(r), std::move(handler));
   }

   /// Async POST
   void post(const std::string& resource,
          const std::string& data,
          ClientResponseHandler handler,
          const std::string& contentType="application/json")

   {
      ClientRequest r("POST", resource);
      r.content(data);
      r.setHeaderContentType(contentType);

      execute(std::move(r), std::move(handler));
   }

   /// Async PUT
   void put(const std::string& resource,
          const std::string& data,
          ClientResponseHandler handler,
          const std::string& contentType="application/json")

   {
      ClientRequest r("PUT", resource);
      r.content(data);
      r.setHeaderContentType(contentType);

      execute(std::move(r), std::move(handler));
   }


   // ---------------------------------------
   // Synchronous requests
   // ---------------------------------------

   /// Sync GET
   ClientResponsePtr get(const std::string& resource)
   {
      ClientRequest r("GET", resource);
      return execute(std::move(r));
   }

   /// Sync DEL
   ClientResponsePtr del(const std::string& resource)
   {
      ClientRequest r("DEL", resource);
      return execute(std::move(r));
   }

   /// Sync POST
   ClientResponsePtr post(const std::string& resource,
                  const std::string& data,
                  const std::string& contentType="application/json")
   {
      ClientRequest r("POST", resource);
      r.content(data);
      r.setHeaderContentType(contentType);

      return execute(std::move(r));
   }

   /// Sync PUT
   ClientResponsePtr put(const std::string& resource,
                  const std::string& data,
                  const std::string& contentType="application/json")
   {
      ClientRequest r("PUT", resource);
      r.content(data);
      r.setHeaderContentType(contentType);

      return execute(std::move(r));
   }


   // ---------------------------------------

   /// Perform synchronous request
   ClientResponsePtr execute(ClientRequest req)
   {
      ensureMode(Mode::Sync);

      if (!_internalIoCtx)
         throw std::runtime_error("Client: sync request requires internal io_context");

      if (_workGuard)
         throw std::logic_error("Client: cannot use sync request while async run() is active");

      ClientResponsePtr resp;
      ioContext().restart();

      asio::post(_clientExecutor/*ioContext()*/, 
         [this, &resp, req=std::move(req)]() mutable
      {
         try
         {
            doExecute(std::move(req),
               [&](ClientResponsePtr response) {
                  resp = std::move(response);
                  // we got response, immediately stop io_context
                  // thus stopping client http connection, and finished the running thread
                  ioContext().stop();
               });
         }
         catch(const std::exception& ex)
         {
            _logger.trace("[{}] Error occured when executing HTTP client execute : {}", logHttpType(), ex.what());
            ioContext().stop();
         }
      });

      ioContext().run();
      return std::move(resp);  
   }

   /// Perform asynchronous request
   void execute(ClientRequest req, ClientResponseHandler handler)
   {
      ensureMode(Mode::Async);

      asio::dispatch(_clientExecutor/*ioContext()*/, 
         [this, req=std::move(req), hndlr = std::move(handler)]() mutable
      {
         try
         {
            auto conn = acquireConnection();
            if (conn)
            {
               sendRequest(conn, std::move(req));
               return;
            }

            if (_activeConnections < _maxConnections)
            {
               doExecute(std::move(req), std::move(hndlr));
               return;
            }

            _pendingRequests.emplace_back(std::move(req), std::move(hndlr));

            //_logger.debug("[{}] request queued (pending={})", logHttpType(), _pendingRequests.size());
         }
         catch(const std::exception& ex)
         {
            _logger.trace("[{}] Error occured when executing HTTP client execute : {}", logHttpType(), ex.what());
            ioContext().stop();
         }
      });
   }

   /// Run client in async mode
   /// Async request is non-blocking, must call run()
   void run(std::size_t threads = 1)
   {
      _logger.debug("[{}] Run with {} thread", logHttpType(), threads );

      if (_workGuard)
         throw std::logic_error("Client: run() already called");

      _workGuard = std::make_unique<WorkGuard>(_pIoContext->get_executor());

      ioContext().restart();
      if (threads <= 1)
      {
         ioContext().run();
         return;
      }

      std::vector<std::thread> workers;
      workers.reserve(threads);

      for (size_t i = 0; i < threads; ++i)
      {
         workers.emplace_back([this]
         {
            ioContext().run();
         });
      }

      for (auto& t : workers)
         t.join();
   }

   void stop()
   {
      _connector.stop();
      _connManager.stopAll();

      if (_workGuard)
         _workGuard.reset();
   }

   size_t totalConnections() 
   {
      return _connManager.totalConnections();
   }

protected:

   void handleConnectionTimeOut(const TimeOutData& data)
   {
      if (_onConnectionTimeOut)
         _onConnectionTimeOut(data);
   }

   void handleConnectionError(const http::ErrorData& error)
   {
      if (_onConnectionError)
         _onConnectionError(error);
   }

   void handleConnectionClosed(ConnectionId connId)
   {
      _activeConnections--;
      if (_onConnectionClosed)
         _onConnectionClosed(connId);
   }

   void handleConnectionFailed(const std::string& message)
   {
      _activeConnections--;
      if (_onConnectFailed)
         _onConnectFailed(message);
   }

   virtual void doExecute(
      ClientRequest req,
      ClientResponseHandler handler = nullptr)
   {
      auto conn = acquireConnection();
      if (conn)
      {
         sendRequest(conn, std::move(req));
         return;
      }

      _activeConnections++;  

      _connector.start(handler,
         // OnConnectionCreated handler
         [this, req=std::move(req)] (ConnectionPtr conn)
         {
            auto request = buildRequest(std::move(req));
            conn->markBusy();
            conn->request(std::move(request));
            
            conn->responseHandledCb([this,conn=conn]() 
            {
               conn->markIdle();
               releaseConnection(conn);
            });

            _connManager.addConnection( std::move(conn) );
         },
         // OnConnectFailed handler
         [this] (const std::string& message)
         {
            handleConnectionFailed(message);
         }
      );
   }

private:
   std::string logHttpType() const
   {
      return logHttpTypeInfo(InstanceType::http_client, _settings.tlsMode());
   }
};

/** @}*/

} // namespace http
} // namespace tbs