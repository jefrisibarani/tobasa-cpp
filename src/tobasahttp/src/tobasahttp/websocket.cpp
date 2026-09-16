/*
   Adapted from Simple-WebSocket-Server by Christian Eidheim
   https://gitlab.com/eidheim/Simple-WebSocket-Server
*/

#include <tobasa/logger.h>
#include <tobasahttp/util.h>
#include "tobasahttp/websocket.h"

namespace tbs {
namespace http {

namespace ws {

void WebSocketState::onOpen()
{
   if (!wsContext || !wsPtr)
      return;

   wsContext->addConnection(wsPtr);

   if (wsContext->onOpen)
      wsContext->onOpen(wsPtr);
}

void WebSocketState::onClose(int32_t status, const std::string& reason)
{
   if (closed || !wsContext || !wsPtr)
      return;

   closed = true;
   auto connection = wsPtr;
   auto context = wsContext;

   if (context->onClose)
      context->onClose(connection, status, reason);

   context->stop(connection, reason);
   wsPtr.reset();
   wsContext.reset();
   fragmentedInMessage.reset();
}

void WebSocketState::onPing()
{
   if (wsContext && wsPtr && wsContext->onPing)
      wsContext->onPing(wsPtr);
}

void WebSocketState::onPong()
{
   if (wsContext && wsPtr && wsContext->onPong)
      wsContext->onPong(wsPtr);
}

void WebSocketState::onMessage(const std::string& message)
{
   if (wsContext && wsPtr && wsContext->onMessage)
      wsContext->onMessage(wsPtr, message);
}

void WebSocketState::onError(const ErrorData& error)
{
   if (closed || !wsContext || !wsPtr)
      return;

   closed = true;
   auto connection = wsPtr;
   auto context = wsContext;

   if (context->onError)
      context->onError(connection, error);
   
   context->stop(connection, error.message);
   wsPtr.reset();
   wsContext.reset();
   sendQueue.clear();
   fragmentedInMessage.reset();
}

void WebSocketState::onError(const std::error_code& error, ErrorType errorTpe, const std::string& source)
{
   if (closed || !wsContext || !wsPtr)
      return;

   closed = true;
   auto connection = wsPtr;
   auto context = wsContext;

   if (context->onError)
   {
      ErrorData err;
      err.code    = error.value();
      err.message = error.message();
      err.connId  = connection->id();
      err.type    = errorTpe;
      err.source  = source;

      context->onError(connection, err);
   }

   context->stop(connection, error.message());
   wsPtr.reset();
   wsContext.reset();
   sendQueue.clear();
   fragmentedInMessage.reset();
}

} // namespace ws

WebSocket::WebSocket(ConnectionPtr conn, 
      const std::any& userData, 
      const asio::ip::tcp::endpoint& ep, 
      Headers& requestHeader)
   : _connection {conn}
   , _userData {userData}
   , _remoteEndpoint {std::move(ep)}
   , _requestHeaders {requestHeader}
{}

void WebSocket::sendText(const std::string& data, WsSendErrorHandler callback)
{
   auto connection = _connection.lock();
   if (!connection || connection->closed())
   {
      Logger::logT("[websocket] Connection already closed");
      return;
   }
   
   auto sender = std::dynamic_pointer_cast<WebSocketSender>(connection);
   if (sender)
      sender->wsSendText(data, callback);
   else
      Logger::logT("[websocket] Connection {} does not support wsSendText", connection->id());
}

void WebSocket::sendBinary(const std::string& data, WsSendErrorHandler callback)
{
   auto connection = _connection.lock();
   if (!connection || connection->closed())
   {
      Logger::logT("[websocket] Connection already closed");
      return;
   }

   auto sender = std::dynamic_pointer_cast<WebSocketSender>(connection);
   if (sender)
      sender->wsSendBinary(data, callback);
   else
      Logger::logT("[websocket] Connection {} does not support wsSendBinary", connection->id());
}

void WebSocket::close(const std::string& reason, int32_t closeCode)
{
   auto connection = _connection.lock();
   if (!connection)
      return;

   auto sender = std::dynamic_pointer_cast<WebSocketSender>(connection);
   if (sender)
      sender->wsSendClose(closeCode, reason);
   else
      Logger::logT("[websocket] Connection {} does not support wsSendClose", connection->id());

   // Use callClose() to ensure the ConnectionManager properly handles
   // the closure while the underlying connection is still available.
   connection->callClose(reason);
}

ConnectionId WebSocket::id() const
{
   if (auto connection = _connection.lock())
      return connection->id();

   return 0;
}

std::string WebSocket::identifier() const 
{ 
   return _identifier; 
}

asio::ip::tcp::endpoint WebSocket::remoteEndpoint() const
{ 
   return _remoteEndpoint; 
}

bool WebSocket::closed() const
{
   if (auto connection = _connection.lock())
      return connection->closed();

   return true;
}

void WebSocket::identifier(const std::string& id) 
{ 
   if (auto connection = _connection.lock())
      connection->identifier(id);

   _identifier = id;
}

std::any& WebSocket::userData() 
{ 
   return _userData; 
}

Headers& WebSocket::requestHeaders() 
{ 
   return _requestHeaders; 
}


WebSocketContext::~WebSocketContext()
{
   std::lock_guard<std::mutex> lock(_connectionsMutex);
   _connections.clear();
}

void WebSocketContext::addConnection(WebSocketPtr sock)
{
   std::lock_guard<std::mutex> lock(_connectionsMutex);
   _connections.emplace(std::move(sock));
}

/// Stop the specified connection.
void WebSocketContext::stop(WebSocketPtr sock, const std::string& reason)
{
   std::size_t connectionCount;
   {
      std::lock_guard<std::mutex> lock(_connectionsMutex);
      _connections.erase(sock);
      connectionCount = _connections.size();
   }

   if (!reason.empty()) {
      Logger::logD("[websocket] Closing web socket client id: {} reason: {}", sock->id(), reason);
   }

   Logger::logD("[websocket] Total web socket client: {}", connectionCount);
}

void WebSocketContext::stop(ConnectionId id, const std::string& reason)
{
   for (auto sock: connectionsSnapshot())
   {
      if (sock->id() == id)
      {
         stop(sock, reason);
         return;
      }
   }
}

void WebSocketContext::close(ConnectionId id, const std::string& reason, int32_t closeCode)
{
   for (auto sock: connectionsSnapshot())
   {
      if (sock->id() == id)
      {
         stop(sock, reason);

         // terminate underlying socket
         sock->close(reason, closeCode);

         return;
      }
   }
}

bool WebSocketContext::isClientConnected(const std::string& identifier) const
{
   for (const auto& conn : connectionsSnapshot())
   {
      if (conn->identifier() == identifier && !conn->closed())
         return true;
   }

   return false;
}

WebSocketPtr WebSocketContext::findClient(const std::string& identifier) const
{
   for (auto conn: connectionsSnapshot())
   {
      if (conn->identifier() == identifier) {
         return conn;
      }
   }
   return nullptr;
}

bool WebSocketContext::hasClient() const
{
   std::lock_guard<std::mutex> lock(_connectionsMutex);
   return !_connections.empty();
}

void WebSocketContext::sendText(const std::string& data, ConnectionId connId, WsSendErrorHandler callback)
{
   for (auto conn: connectionsSnapshot())
   {
      if (connId==0)
         conn->sendText(data, callback);
      else if (conn->id() == connId)
         conn->sendText(data, callback);
   }
}

void WebSocketContext::sendBinary(const std::string& data, ConnectionId connId, WsSendErrorHandler callback)
{
   for (auto conn: connectionsSnapshot())
   {
      if (connId==0)
         conn->sendBinary(data,callback);
      else if (conn->id() == connId)
         conn->sendBinary(data,callback);
   }
}

void WebSocketContext::sendText(const std::string& data, const std::string& identifier, 
      WsSendErrorHandler callback, ws::SkipSendHandler skipCallback )
{
   for (auto conn: connectionsSnapshot())
   {
      if (skipCallback)
      {
         if (skipCallback(conn->id()))
            continue;
      }

      if (identifier.empty())
         conn->sendText(data, callback);
      else if (conn->identifier() == identifier)
         conn->sendText(data, callback);
   }
}

void WebSocketContext::sendBinary(const std::string& data, const std::string& identifier, 
      WsSendErrorHandler callback, ws::SkipSendHandler skipCallback )
{
   for (auto conn: connectionsSnapshot())
   {
      if (skipCallback)
      {
         if (skipCallback(conn->id()))
            continue;
      }

      if (identifier.empty())
         conn->sendBinary(data, callback);
      else if (conn->identifier() == identifier)
         conn->sendBinary(data, callback);
   }
}

std::vector<WebSocketPtr> WebSocketContext::connectionsSnapshot() const
{
   std::lock_guard<std::mutex> lock(_connectionsMutex);
   return {_connections.begin(), _connections.end()};
}

} // namespace http
} // namespace tbs