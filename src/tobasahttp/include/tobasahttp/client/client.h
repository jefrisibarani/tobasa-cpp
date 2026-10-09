#pragma once

#include <set>
#include <vector>
#include <string>
#include <deque>
#include <mutex>
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

      // Call shutdown() after submitting the batch. The client drains all
      // submitted requests before run() returns.

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
            auto result = client.get( "/api/version");
            if (result.responseReceived())
               processResponse(result.response);
            else
               logger.error("Request failed: {}", result.error.message);
         }
      }
      else
      {
         // run 100 Async request
         std::string resource = "/api/version";
         for (int i=0;i<100;i++)
         {
            client.get("/api/version", [&](const http::ClientResult& result) {
               if (result.responseReceived())
                  processResponse(result.response);
               else
                  logger.error("Request failed: {}", result.error.message);
            });
         }

         client.shutdown();
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
   IoContextPtr               _pIoContext;
   Settings                   _settings;
   Logger&                    _logger;
   ConnectionMgr<Traits>      _connManager;
   Connector<Traits>          _connector;
   Executor                   _clientExecutor;
   std::unique_ptr<WorkGuard> _workGuard;
   std::mutex                 _modeMutex;
   std::mutex                 _callbackMutex;

   OnConnectFailed            _onConnectFailed;
   OnConnectionTimeOut        _onConnectionTimeOut;
   OnConnectionError          _onConnectionError;
   OnConnectionClosed         _onConnectionClosed;
   
   bool                       _internalIoCtx;

   std::deque<ConnectionPtr>  _idleConnections;
   size_t                     _maxConnections    {2};
   size_t                     _activeConnections {0};

   Headers                    _defaultHeaders;

   struct PendingRequest
   {
      ClientRequest              request;
      ClientResponseHandler      responseHandler;
      ClientRequestErrorHandler  errorHandler;
   };

   std::deque<PendingRequest>    _pendingRequests;

   enum class Mode 
   {
      None,
      Sync,
      Async
   };

   Mode   _mode                {Mode::None};
   bool   _shutdownRequested   {false};
   bool   _shutdownComplete    {false};
   size_t _outstandingRequests {0};

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

   
   /**
    * Gives a completed connection to the oldest pending request, or returns
    * it to the idle pool when no request is waiting. Called on the client
    * strand.
    */
   void releaseConnection(ConnectionPtr conn)
   {
      if (!_pendingRequests.empty())
      {
         auto pending = std::move(_pendingRequests.front());
         _pendingRequests.pop_front();

         sendRequest(conn, std::move(pending.request),
            std::move(pending.responseHandler), std::move(pending.errorHandler));

         return;
      }

      if (_idleConnections.size() < _maxConnections)
      {
         _logger.info("[{}] releasing connection", logHttpType());
         _idleConnections.push_back(std::move(conn));
      }
   }

   /**
    * Converts a ClientRequest into the HTTP/1 request sent on the socket.
    * Adds Host and keep-alive headers, then copies the client defaults and
    * request-specific headers.
    */
   std::unique_ptr<ClientRequest> buildRequest(ClientRequest req)
   {
      auto httpReq = std::make_unique<ClientRequest>(std::move(req));

      std::string host = tbsfmt::format("{}:{}", _settings.address(), _settings.port());
      httpReq->addHeader("Host", host);
      httpReq->addHeader("Connection", "keep-alive");

      for (auto h : _defaultHeaders.getValues() ) {
         httpReq->addHeader(h->name(), h->value());
      }

      for (auto h: req.headers().getValues()) {
         httpReq->addHeader(h->name(), h->value());
      }

      return httpReq;
   }

   /**
    * Builds and sends a request on an existing connection. Marks the
    * connection busy and passes the request's response handler to it.
    * Called on the client strand.
    */
   void sendRequest(
      ConnectionPtr conn,
      ClientRequest req,
      ClientResponseHandler handler,
      ClientRequestErrorHandler errorHandler = nullptr)
   {
      auto request = buildRequest(std::move(req));
      conn->markBusy();
      conn->sendRequest(std::move(request), std::move(handler), std::move(errorHandler));
   }

   void ensureMode(Mode requested)
   {
      std::lock_guard<std::mutex> lock(_modeMutex);
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

   void ensureAsyncRequestAccepted()
   {
      std::lock_guard<std::mutex> lock(_modeMutex);
      if (_shutdownRequested)
         throw std::logic_error("Client: cannot submit requests after shutdown has started");

      if (_mode == Mode::None)
         _mode = Mode::Async;
      else if (_mode != Mode::Async)
         throw std::logic_error("Client: cannot mix synchronous and asynchronous requests");

      ++_outstandingRequests;
   }

   void clearCallbacks()
   {
      std::lock_guard<std::mutex> lock(_callbackMutex);
      _onConnectFailed     = nullptr;
      _onConnectionTimeOut = nullptr;
      _onConnectionError   = nullptr;
      _onConnectionClosed  = nullptr;
   }

   void finishAsyncShutdown()
   {
      {
         std::lock_guard<std::mutex> lock(_modeMutex);
         if (!_shutdownRequested || _shutdownComplete || _outstandingRequests != 0)
            return;

         _shutdownComplete = true;
      }

      stop();
      clearCallbacks();
   }

   void completeAsyncRequest()
   {
      bool shouldShutdown = false;
      {
         std::lock_guard<std::mutex> lock(_modeMutex);
         if (_outstandingRequests > 0)
            --_outstandingRequests;

         shouldShutdown = _shutdownRequested && _outstandingRequests == 0;
      }

      if (shouldShutdown)
         finishAsyncShutdown();
   }

   void resetAsyncShutdownAfterRun()
   {
      std::lock_guard<std::mutex> lock(_modeMutex);
      if (_shutdownComplete)
      {
         _mode = Mode::None;
         _shutdownRequested = false;
         _shutdownComplete = false;
      }
   }


   /// Get executor/strand, in single threaded, the type is asio::any_io_executor.
   /// in multithreaded the type is asio::strand<asio::any_io_executor>
   auto & executor() noexcept { return _clientExecutor; }

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
         asio::post(_clientExecutor, [this, data] {
            handleConnectionTimeOut(data);
         });
      });
      
      _connManager.onConnectionError([this](const http::ErrorData& error) {
         asio::post(_clientExecutor, [this, error] {
            handleConnectionError(error);
         });
      });

      _connManager.onConnectionClosed([this](ConnectionId connId) {
         asio::post(_clientExecutor, [this, connId] {
            handleConnectionClosed(connId);
         });
      });

      _maxConnections = _settings.connectionPoolSize();
   }

   /**
    * Shuts down the client. In asynchronous mode, this is non-blocking: it
    * rejects new requests, drains all accepted requests and their result
    * handlers, then closes the connections so run() can return. Call it after
    * submitting the requests to process. Outside asynchronous mode, shutdown
    * stops the client immediately. Use stop() to cancel asynchronous work.
    */
   void shutdown()
   {
      bool asyncMode = false;

      {
         std::lock_guard<std::mutex> lock(_modeMutex);
         if (_shutdownRequested)
            return;

         _shutdownRequested = true;
         asyncMode = _mode == Mode::Async;
      }

      if (asyncMode)
      {
         asio::dispatch(_clientExecutor, [this] {
            bool shouldShutdown = false;
            
            {
               std::lock_guard<std::mutex> lock(_modeMutex);
               shouldShutdown = _outstandingRequests == 0;
            }

            if (shouldShutdown)
               finishAsyncShutdown();
         });

         return;
      }

      stop();

      if (_internalIoCtx && _pIoContext)
      {
         if (!_pIoContext->stopped())
            _pIoContext->stop();
      }

      clearCallbacks();
      {
         std::lock_guard<std::mutex> lock(_modeMutex);
         _mode = Mode::None;
         _shutdownRequested = false;
      }
   }


   virtual ~Client()
   {
      stop();

      if (_internalIoCtx && _pIoContext && !_pIoContext->stopped())
         _pIoContext->stop();

      clearCallbacks();

      _logger.trace("[{}] Client destroyed", logHttpType());
   }


   /// Sets an optional global callback for failed connection attempts.
   /// The associated request also receives the failure through its ClientResult.
   void onConnectFailed(OnConnectFailed handler)
   {
      std::lock_guard<std::mutex> lock(_callbackMutex);
      _onConnectFailed = std::move(handler);
   }

   /// Sets an optional global callback for connection timeouts.
   void onConnectionTimeOut(OnTimeOut handler)
   {
      std::lock_guard<std::mutex> lock(_callbackMutex);
      _onConnectionTimeOut = std::move(handler);
   }

   /// Sets an optional global callback for connection errors.
   void onConnectionError(OnConnectionError handler)
   {
      std::lock_guard<std::mutex> lock(_callbackMutex);
      _onConnectionError = std::move(handler);
   }

   /// Sets an optional global callback when a managed connection closes.
   void onConnectionClosed(OnConnectionClosed handler)
   {
      std::lock_guard<std::mutex> lock(_callbackMutex);
      _onConnectionClosed = std::move(handler);
   }

   /// Adds a default header to every request sent by this client.
   void addHeader(const std::string& key,const std::string& value)
   {
      _defaultHeaders.add(key, value);
   }

   // ---------------------------------------
   // Asynchronous requests
   // ---------------------------------------

   /// Async GET
   void get(const std::string& resource, ClientResultHandler handler)
   {
      ClientRequest r("GET", resource);
      execute(std::move(r), std::move(handler));
   }

   /// Async HEAD
   void head(const std::string& resource, ClientResultHandler handler)
   {
      ClientRequest r("HEAD", resource);
      execute(std::move(r), std::move(handler));
   }

   /// Async OPTIONS
   void options(const std::string& resource, ClientResultHandler handler)
   {
      ClientRequest r("OPTIONS", resource);
      execute(std::move(r), std::move(handler));
   }

   /// Async DEL
   void del(const std::string& resource, ClientResultHandler handler)
   {
      ClientRequest r("DELETE", resource);
      execute(std::move(r), std::move(handler));
   }

   /// Async POST
   void post(const std::string& resource,
          const std::string& data,
          ClientResultHandler handler,
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
          ClientResultHandler handler,
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
   ClientResult get(const std::string& resource)
   {
      ClientRequest r("GET", resource);
      return execute(std::move(r));
   }

   /// Sync HEAD
   ClientResult head(const std::string& resource)
   {
      ClientRequest r("HEAD", resource);
      return execute(std::move(r));
   }

   /// Sync OPTIONS
   ClientResult options(const std::string& resource)
   {
      ClientRequest r("OPTIONS", resource);
      return execute(std::move(r));
   }

   /// Sync DEL
   ClientResult del(const std::string& resource)
   {
      ClientRequest r("DELETE", resource);
      return execute(std::move(r));
   }

   /// Sync POST
   ClientResult post(const std::string& resource,
                  const std::string& data,
                  const std::string& contentType="application/json")
   {
      ClientRequest r("POST", resource);
      r.content(data);
      r.setHeaderContentType(contentType);

      return execute(std::move(r));
   }

   /// Sync PUT
   ClientResult put(const std::string& resource,
                  const std::string& data,
                  const std::string& contentType="application/json")
   {
      ClientRequest r("PUT", resource);
      r.content(data);
      r.setHeaderContentType(contentType);

      return execute(std::move(r));
   }

   
   /**
    * Executes a synchronous request and returns either its response or the
    * error that prevented a response from being received.
    * The client must not be running in asynchronous mode.
    */
   ClientResult execute(ClientRequest req)
   {
      ensureMode(Mode::Sync);

      if (!_internalIoCtx)
         throw std::runtime_error("Client: sync request requires internal io_context");

      if (_workGuard)
         throw std::logic_error("Client: cannot use sync request while async run() is active");

      ClientResult result;

      ioContext().restart();

      asio::post(_clientExecutor,
         [this, &result, req=std::move(req)]() mutable
      {
         auto responseHandler = [this, &result](const ClientResponsePtr& response)
         {
            result.response = response;
            // we got response, immediately stop io_context
            // thus stopping client http connection, and finished the running thread
            ioContext().stop();
         };

         auto errorHandler = [this, &result](const ErrorData& error)
         {
            result.error = error;
         };

         try
         {
            doExecute(std::move(req), std::move(responseHandler), std::move(errorHandler));
         }
         catch (const std::exception& ex)
         {
            result.error = ErrorData{ ex.what(), ErrorType::exception, ERROR_CODE_EXCEPTION, 0, "Client" };
            ioContext().stop();
         }
      });

      ioContext().run();

      return result;
   }

   /**
    * Schedules a request and reports either its response or request error to
    * the supplied handler. Call run() to process the I/O.
    */
   void execute(ClientRequest req, ClientResultHandler handler)
   {
      ensureAsyncRequestAccepted();

      auto deliverResult = [this, handler](ClientResult result)
      {
         if (handler)
         {
            try
            {
               handler(result);
            }
            catch (...)
            {
               asio::dispatch(_clientExecutor, [this] { completeAsyncRequest(); });
               throw;
            }
         }
         asio::dispatch(_clientExecutor, [this] { completeAsyncRequest(); });
      };

      auto responseHandler = [deliverResult](const ClientResponsePtr& response)
      {
         deliverResult(ClientResult{response, {}});
      };

      auto errorHandler = [deliverResult](const ErrorData& error)
      {
         deliverResult(ClientResult{nullptr, error});
      };

      // Run inline if already on the client strand; otherwise schedule there.
      asio::dispatch(_clientExecutor,
       [this, req = std::move(req),
        responseHandler = std::move(responseHandler),
        errorHandler    = std::move(errorHandler),
        deliverResult   = std::move(deliverResult)]() mutable
      {
         try
         {
            auto conn = acquireConnection();
            if (conn)
            {
               sendRequest(conn, std::move(req), std::move(responseHandler), std::move(errorHandler));
               return;
            }

            if (_activeConnections < _maxConnections)
            {
               doExecute(std::move(req), std::move(responseHandler), std::move(errorHandler));
               return;
            }

            _pendingRequests.push_back(PendingRequest{
               std::move(req), std::move(responseHandler), std::move(errorHandler)});
         }
         catch (const std::exception& ex)
         {
            deliverResult(ClientResult{nullptr, ErrorData{
               ex.what(), ErrorType::exception, ERROR_CODE_EXCEPTION, 0, "Client"}});
         }
      });
   }

   /**
   * Runs the asynchronous client's I/O loop and blocks until it stops. Uses
   * the calling thread when threads <= 1; otherwise starts and joins the
   * requested number of workers. shutdown() drains submitted requests and
   * then stops the loop.
    */
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
         resetAsyncShutdownAfterRun();
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

      resetAsyncShutdownAfterRun();
   }

   
   /**
   * Immediately cancels connection attempts and closes managed connections.
   * Unlike shutdown(), this does not drain pending requests or explicitly
   * stop the client's I/O context.
    */
   void stop()
   {
      _connector.stop();
      _connManager.stopAll();

      if (_workGuard)
         _workGuard.reset();
   }

   /// Returns the number of managed connections.
   size_t totalConnections() 
   {
      return _connManager.totalConnections();
   }

protected:

   void handleConnectionTimeOut(const TimeOutData& data)
   {
      OnTimeOut handler;

      {
         std::lock_guard<std::mutex> lock(_callbackMutex);
         handler = _onConnectionTimeOut;
      }

      if (handler)
         handler(data);
   }

   void handleConnectionError(const http::ErrorData& error)
   {
      OnConnectionError handler;

      {
         std::lock_guard<std::mutex> lock(_callbackMutex);
         handler = _onConnectionError;
      }

      if (handler)
         handler(error);
   }

   void handleConnectionClosed(ConnectionId connId)
   {
      if (_activeConnections > 0)
         _activeConnections--;

      while (!_pendingRequests.empty() && _activeConnections < _maxConnections)
      {
         auto pending = std::move(_pendingRequests.front());
         _pendingRequests.pop_front();

         doExecute(std::move(pending.request),
            std::move(pending.responseHandler), std::move(pending.errorHandler));
      }

      OnConnectionClosed handler;
      {
         std::lock_guard<std::mutex> lock(_callbackMutex);
         handler = _onConnectionClosed;
      }
      if (handler)
         handler(connId);
   }

   void handleConnectionFailed(
      const std::string& message,
      ClientRequestErrorHandler errorHandler = nullptr)
   {
      if (_activeConnections > 0)
         _activeConnections--;

      OnConnectFailed handler;
      {
         std::lock_guard<std::mutex> lock(_callbackMutex);
         handler = _onConnectFailed;
      }
      if (handler)
         handler(message);

      if (errorHandler)
      {
         errorHandler(ErrorData{
            message, ErrorType::system, ERROR_CODE_DATA_FAIL, 0, "Connector"});
      }

      while (!_pendingRequests.empty() &&
             _activeConnections < _maxConnections &&
             !ioContext().stopped())
      {
         auto pending = std::move(_pendingRequests.front());
         _pendingRequests.pop_front();

         doExecute(std::move(pending.request),
            std::move(pending.responseHandler), std::move(pending.errorHandler));
      }
   }


   /**
    * Sends a request on an idle connection or starts a new connection.
    * New connections are resolved and connected by Connector, then registered
    * with the connection manager before their first request is sent. This
    * method runs on the client strand, which owns the pool state.
    */
   void doExecute(
      ClientRequest req,
      ClientResponseHandler handler,
      ClientRequestErrorHandler errorHandler)
   {
      auto conn = acquireConnection();
      if (conn)
      {
         sendRequest(conn, std::move(req), std::move(handler), std::move(errorHandler));
         return;
      }

      _activeConnections++;  

      _connector.start({},
         // OnConnectionCreated handler
         [this, req=std::move(req), handler=std::move(handler), errorHandler]
         (ConnectionPtr conn) mutable
         {
            asio::post(_clientExecutor,
               [this, conn=std::move(conn), req=std::move(req),
                handler=std::move(handler), errorHandler=std::move(errorHandler)]() mutable
            {
               auto request = buildRequest(std::move(req));
               conn->markBusy();
               conn->request(std::move(request), std::move(handler), std::move(errorHandler));

               conn->responseHandledCb([this, conn]
               {
                  asio::post(_clientExecutor, [this, conn]
                  {
                     if (conn->closed())
                        return;

                     conn->markIdle();
                     releaseConnection(conn);
                  });
               });

               _connManager.addConnection(std::move(conn));
            });
         },
         // OnConnectFailed handler
         [this, errorHandler] (const std::string& message)
         {
            asio::post(_clientExecutor, [this, message, errorHandler]
            {
               handleConnectionFailed(message, errorHandler);
            });
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