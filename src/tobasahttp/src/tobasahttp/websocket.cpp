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

void WebSocketConn::onOpen()
{
   wsContext->addConnection(wsPtr);

   if (wsContext->onOpen)
      wsContext->onOpen(wsPtr);
}

void WebSocketConn::onClose(int32_t status, const std::string& reason)
{
   if (wsContext->onClose)
      wsContext->onClose(wsPtr, status, reason);

   wsContext->stop(wsPtr, reason);
}

void WebSocketConn::onPing()
{
   if (wsContext->onPing)
      wsContext->onPing(wsPtr);
}

void WebSocketConn::onPong()
{
   if (wsContext->onPong)
      wsContext->onPong(wsPtr);
}

void WebSocketConn::onMessage(const std::string& message)
{
   if (wsContext->onMessage)
      wsContext->onMessage(wsPtr, message);
}

void WebSocketConn::onError(const ErrorData& error)
{
   if (wsContext->onError)
      wsContext->onError(wsPtr, error);
   
   wsContext->stop(wsPtr, error.message);
}

void WebSocketConn::onError(const std::error_code& error, ErrorType errorTpe, const std::string& source)
{
   if (wsContext->onError)
   {
      ErrorData err;
      err.code    = error.value();
      err.message = error.message();
      err.connId  = wsPtr->id();
      err.type    = errorTpe;
      err.source  = source;

      wsContext->onError(wsPtr, err);
   }

   wsContext->stop(wsPtr, error.message());
}

} // namespace ws

WebSocket::WebSocket(ConnectionPtr connection, std::any& userData, const asio::ip::tcp::endpoint& ep, Headers& requestHeader)
   : _connection {connection}
   , _userData   {userData}
   , _remoteEndpoint {std::move(ep)}
   , _requestHeaders {requestHeader}
{
}

void WebSocket::sendText(const std::string& data, WsSendErrorHandler callback)
{
   if (_connection->closed())
   {
      Logger::logT("[websocket] Connection {} already closed", _connection->id());
      return;
   }
   _connection->wsSendText(data, callback);
}

void WebSocket::sendBinary(const std::string& data, WsSendErrorHandler callback)
{
   if (_connection->closed())
   {
      Logger::logT("[websocket] Connection {} already closed", _connection->id());
      return;
   }
   _connection->wsSendBinary(data, callback);
}

void WebSocket::close(const std::string& reason, int32_t closeCode)
{
   _connection->wsSendClose(closeCode, reason);

   // Use callClose() to ensure the ConnectionManager properly handles the closure of this connection.
   _connection->callClose(reason);
}

ConnectionId WebSocket::id()
{
   return _connection->id();
}

bool WebSocket::closed()
{
   return _connection->closed();
}

void WebSocket::identifier(const std::string& id) 
{ 
   _connection->identifier(id);
   _identifier = id;
}

WebSocketContext::~WebSocketContext()
{
   _connections.clear();
}

void WebSocketContext::addConnection(WebSocketPtr sock)
{  
   _connections.emplace(std::move(sock));
}

/// Stop the specified connection.
void WebSocketContext::stop(WebSocketPtr sock, const std::string& reason)
{
   if (!reason.empty()) {
      Logger::logD("[websocket] Closing web socket client id: {} reason: {}", sock->id(), reason);
   }

   _connections.erase(sock);

   Logger::logD("[websocket] Total web socket client: {}", _connections.size());
}

void WebSocketContext::stop(ConnectionId id, const std::string& reason)
{
   for (auto sock: _connections)
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
   for (auto sock: _connections)
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
   // Bacause we allow a client to connect from multiple devices with one identifier.
   // Therefore, check if any device is still connected.
   auto conn = findClient(identifier);
   while (conn != nullptr)
   {
      if (!conn->closed())
         return true;
   }
   return false;
}

WebSocketPtr WebSocketContext::findClient(const std::string& identifier) const
{
   for (auto conn: _connections)
   {
      if (conn->identifier() == identifier) {
         return conn;
      }
   }
   return nullptr;
}

bool WebSocketContext::hasClient() const
{
   return _connections.size() > 0;
}

void WebSocketContext::sendText(const std::string& data, ConnectionId connId, WsSendErrorHandler callback)
{
   for (auto conn: _connections)
   {
      if (connId==0)
         conn->sendText(data, callback);
      else if (conn->id() == connId)
         conn->sendText(data, callback);
   }
}

void WebSocketContext::sendBinary(const std::string& data, ConnectionId connId, WsSendErrorHandler callback)
{
   for (auto conn: _connections)
   {
      if (connId==0)
         conn->sendBinary(data,callback);
      else if (conn->id() == connId)
         conn->sendBinary(data,callback);
   }
}

void WebSocketContext::sendText(const std::string& data, 
      const std::string identifier, 
      WsSendErrorHandler callback,
      ws::SkipSendHandler skipCallback )
{
   for (auto conn: _connections)
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

void WebSocketContext::sendBinary(const std::string& data, 
      const std::string identifier, 
      WsSendErrorHandler callback,
      ws::SkipSendHandler skipCallback )
{
   for (auto conn: _connections)
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

} // namespace http
} // namespace tbs