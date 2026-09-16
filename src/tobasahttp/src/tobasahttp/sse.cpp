#include <tobasa/logger.h>
#include "tobasahttp/util.h"
#include "tobasahttp/connection.h"
#include "tobasahttp/sse.h"

namespace tbs {
namespace http {

SseConnection::SseConnection(SendHandler sendHandler, CloseHandler closeHandler,
      ConnectionPtr connection, 
      const std::any& userData, const asio::ip::tcp::endpoint& ep)
   : _sendHandler  { std::move(sendHandler) }
   , _closeHandler { std::move(closeHandler) }
   , _connection {connection}
   , _userData   {userData}
   , _remoteEndpoint {std::move(ep)}
{
}

/// Send one SSE event. Data containing newlines is emitted as multiple data fields.
void SseConnection::send(std::string_view data, std::string_view event, std::string_view id)
{
   if (!_sendHandler || _closed)
      return;

   std::string message;
   if (!event.empty())
      message += "event: " + std::string(event) + "\n";

   if (!id.empty())
      message += "id: " + std::string(id) + "\n";

   size_t start = 0;
   while (start <= data.size())
   {
      auto end = data.find('\n', start);
      if (end == std::string_view::npos)
         end = data.size();

      message += "data: ";
      message.append(data.substr(start, end - start));
      message += '\n';

      if (end == data.size())
         break;
      start = end + 1;
   }
   message += '\n';
   _sendHandler(std::move(message));
}

/// Send an SSE comment, commonly used as a heartbeat.
void SseConnection::comment(std::string_view text)
{
   if (!_sendHandler || _closed)
      return;

   std::string message { ": " };
   message.append(text);
   message += "\n\n";
   _sendHandler(std::move(message));
}

/// Close the stream and send the terminating HTTP chunk.
void SseConnection::close(const std::string& reason)
{
   if (_closed.exchange(true))
      return;

   if (_closeHandler)
      _closeHandler();

   // Use callClose() to ensure the ConnectionManager properly handles
   // the closure while the underlying connection is still available.
   if (auto connection = _connection.lock())
   {
      if (connection->httpVersion() != HttpVersion::two)
         connection->callClose(reason);
   }
}

ConnectionId SseConnection::id() const
{
   if (auto connection = _connection.lock())
      return connection->id();

   return 0;
}

std::string SseConnection::identifier() const 
{ 
   return _identifier; 
}

asio::ip::tcp::endpoint SseConnection::remoteEndpoint() const
{ 
   return _remoteEndpoint; 
}

bool SseConnection::closed() const 
{ 
   return _closed.load(); 
}

std::any& SseConnection::userData() 
{ 
   return _userData; 
}

void SseConnection::identifier(const std::string& id) 
{ 
   if (auto connection = _connection.lock())
      connection->identifier(id);

   _identifier = id;
}


void SseContext::add(const SseConnectionPtr& conn)
{
   {
      std::lock_guard<std::mutex> lock(_connectionsMutex);
      _connections.emplace(conn);
   }
   if (onOpen)
      onOpen(conn);
}

void SseContext::remove(const SseConnectionPtr& conn)
{
   {
      std::lock_guard<std::mutex> lock(_connectionsMutex);
      _connections.erase(conn);
   }
}

void SseContext::send(std::string_view data, std::string_view event, std::string_view id, const std::string& connIdentifier)
{
   auto connections = connectionsSnapshot();
   for (const auto& conn : connections)
   {
      if (connIdentifier.empty())
         conn->send(data, event, id);
      else if (conn->identifier() == connIdentifier)
         conn->send(data, event, id);
   }
}

void SseContext::comment(std::string_view text, const std::string& connIdentifier)
{
   auto connections = connectionsSnapshot();
   for (const auto& conn : connections)
   {
      if (connIdentifier.empty())
         conn->comment(text);
      else if (conn->identifier() == connIdentifier)
         conn->comment(text);
   }
}

void SseContext::close(ConnectionId connId, const std::string& reason)
{
   auto connections = connectionsSnapshot();
   for (const auto& conn : connections)
   {
      if (conn->id() == connId)
      {
         conn->close(reason);
         return;
      }
   }
}

std::vector<SseConnectionPtr> SseContext::connectionsSnapshot() const
{
   std::vector<SseConnectionPtr> result;
   std::lock_guard<std::mutex> lock(_connectionsMutex);
   result.assign(_connections.begin(), _connections.end());

   return result;
}

} // namespace http
} // namespace tbs