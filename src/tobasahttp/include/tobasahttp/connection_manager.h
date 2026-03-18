#pragma once

#include <atomic>
#include <mutex>
#include <tobasa/non_copyable.h>
#include <tobasa/datetime.h>
#include <tobasa/util.h>
#include "tobasahttp/connection.h"
#include "tobasahttp/util.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/**
 * \brief ConnectionIdStore.
 */
class ConnectionIdStore
{
public:
   ConnectionId newId(InstanceType value, bool tls)
   {
      if (value == InstanceType::http_server)
      {
         if (tls)
            return ++_connIdServerTls;
         else
            return ++_connIdServer;
      }
      else
      {
         if (tls)
            return ++_connIdClientTls;
         else
            return ++_connIdClient;
      }
   }

private:
   inline static std::atomic<uint64_t>  _connIdServer    {0};
   inline static std::atomic<uint64_t>  _connIdServerTls {0};
   inline static std::atomic<uint64_t>  _connIdClient    {0};
   inline static std::atomic<uint64_t>  _connIdClientTls {0};
};

/**
 * \brief Http Connection manager.
 */
template <class Traits>
class ConnectionMgr : private NonCopyable
{
public:
   using Settings          = typename Traits::Settings;
   using Logger            = typename Traits::Logger;
   using ConnectionPtr     = std::shared_ptr<Connection>;

private:

   Settings&               _settings;
   Logger&                 _logger;
   std::set<ConnectionPtr> _connections;
   std::mutex              _connectionsMutex;

   ConnectionIdStore       _connIdStore;

   OnConnectionTimeOut     _onConnectionTimeOut;
   OnConnectionError       _onConnectionError;
   OnConnectionClosed      _onConnectionClosed;

   InstanceType            _instanceType;
   ConnectionId            _lastConnectionId = 0;

public:
   /**
    * \brief Constructor
    */
   ConnectionMgr(Settings& settings, Logger& logger, InstanceType instanceType)
      : _settings(settings)
      , _logger(logger)
      , _instanceType(instanceType)
   {
      _logger.trace("[{}] ConnectionMgr initialized", logHttpType());
   }

   ~ConnectionMgr()
   {
      _onConnectionTimeOut = nullptr;
      _onConnectionError   = nullptr;
      _onConnectionClosed  = nullptr;

      try {
         stopAll();
      } catch(...) {
         // swallow in destructor
      }

      _logger.trace("[{}] ConnectionMgr destroyed", logHttpType());
   }

   /// Add the specified connection to the manager and start it.
   void addConnection(ConnectionPtr conn)
   {
      using namespace std::placeholders;

      conn->onTimeOut(  std::bind(&ConnectionMgr::handleOnTimeOut,  this, _1) );
      conn->onError(    std::bind(&ConnectionMgr::handleOnError,    this, _1) );
      conn->onComplete( std::bind(&ConnectionMgr::handleOnComplete, this, _1, _2) );
      conn->onClosed(   std::bind(&ConnectionMgr::handleOnClosed,   this, _1) );

      ConnectionId newId = _connIdStore.newId(conn->instanceType(), conn->isTls());
      _lastConnectionId = newId;
      conn->id( newId );
      conn->start();

      {
         std::lock_guard<std::mutex> lk(_connectionsMutex);
         _connections.emplace(std::move(conn));
      }
   }


   /**
    * \brief Stop a connection
    * \param id connection id
    */
   void stop(ConnectionId id, const std::string& reason = "")
   {
      std::shared_ptr<Connection> target;

      {
         std::lock_guard<std::mutex> lk(_connectionsMutex);

         for (auto it = _connections.begin(); it != _connections.end(); ++it)
         {
               auto& conn = *it;

               if (conn->id() == id && !conn->closed())
               {
                  target = conn;
                  _connections.erase(it);
                  _logger.trace("[{}] Total HTTP connection size: {}", logHttpType(), _connections.size());
                  break;
               }
         }
      } // unlocked here

      if (!target)
         return;

      if (!reason.empty()) {
         _logger.debug("[{}] {}", logHttpType(), reason);
      }
      // we safe now
      target->close();
   }


   /// Stop all connections.
   void stopAll()
   {
      std::vector<std::shared_ptr<Connection>> conns; // temp storage

      {
         std::lock_guard<std::mutex> lk(_connectionsMutex);

         if (_connections.empty())
            return;
         
         _logger.info("[{}] Stopping all HTTP connections", logHttpType());
         
         conns.assign(_connections.begin(), _connections.end());
         _connections.clear();
      } // unlocked here

      // we safe now
      for (auto& conn : conns) {
         conn->close();
      }
   }


   /// @brief Set OnTimeOut handler
   /// @param handler 
   void onConnectionTimeOut(OnConnectionTimeOut handler) { _onConnectionTimeOut = std::move(handler); }
   
   /// @brief Set OnConnectionError handler
   /// @param handler  
   void onConnectionError(OnConnectionError handler)     { _onConnectionError = std::move(handler);}
   
   /// @brief Set OnConnectionClosed handler
   /// @param handler 
   void onConnectionClosed(OnConnectionClosed handler)   { _onConnectionClosed = std::move(handler); }

   size_t totalConnections() 
   {
      std::lock_guard<std::mutex> lk(_connectionsMutex);
      return _connections.size();
   }

   ConnectionId lastConnectionId() { return _lastConnectionId; }

   std::vector<ConnectionInfo> currentConnectionsInfo()
   {
      std::vector<ConnectionInfo> infos;

      std::lock_guard<std::mutex> lk(_connectionsMutex);
      for (auto conn: _connections)
      {
         ConnectionInfo info;
         info.connId         = conn->id();
         info.closed         = conn->closed();
         info.tls            = conn->isTls();
         info.websocket      = conn->isWebSocket();
         info.remoteEndpoint = http::toString(conn->remoteEndpoint());
         info.identifier     = conn->identifier();
         
         DateTime startTime(conn->startTime());
         DateTime endtime;
         auto interval = (endtime.timePoint() - startTime.timePoint()).count();
         
         info.startTime      = startTime.isoDateTimeString();
         info.duration       = util::readMilliseconds(interval);
         infos.emplace_back(std::move(info));
      }

      return std::move(infos);
   }

protected:

   // ConnectionPtr connection(ConnectionId id)
   // {
   //    std::lock_guard<std::mutex> lk(_connectionsMutex);
   //    for (auto conn: _connections)
   //    {
   //       if (conn->id() == id)
   //       {
   //          return conn;
   //       }
   //    }
   //    return nullptr;
   // }

   void handleOnTimeOut(const TimeOutData& data)
   {
      // Note: stopping connection due OnTimeOut event also triggering OnError event
      std::string reason = tbsfmt::format("Stopping HTTP connection ID {} due to {} timeout", data.connId, timerTypeToString(data.timerType));
      stop(data.connId, reason);

      // Handler should not blocking
      if (_onConnectionTimeOut)
         _onConnectionTimeOut(data);
   }

   void handleOnError(const ErrorData& error)
   {
      std::string reason;
      if (error.message == "End of file")
         reason = tbsfmt::format("Stopping HTTP connection ID {} : {}", error.connId, error.message);
      else
         reason = tbsfmt::format("Stopping HTTP connection ID {} error: {}", error.connId, error.message);

      stop(error.connId, reason);

      // Note: OnTimeOut event might already closed the connection
      if (_onConnectionError)
         _onConnectionError(error);
   }

   void handleOnComplete(ConnectionId connId, const std::string& message)
   {
      std::string reason = tbsfmt::format("Stopping HTTP connection with ID {} due to completion, message: {}", connId, message);
      stop(connId, reason);
   }

   void handleOnClosed(ConnectionId connId)
   {
      // Note: Handler should not blocking
      if (_onConnectionClosed)
         _onConnectionClosed(connId);
   }

private:
   std::string logHttpType() const
   {
      return logHttpTypeInfo(_instanceType, _settings.tlsMode());
   }

};

/** @}*/

} // namespace http
} // namespace tbs