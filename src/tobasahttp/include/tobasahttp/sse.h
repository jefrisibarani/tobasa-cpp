#pragma once

#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <mutex>
#include <asio/ip/tcp.hpp>
#include "tobasahttp/type_common.h"

namespace tbs {
namespace http {

namespace sse {

struct SseState
{
   std::deque<std::string> sendQueue;
   bool                    headersWritten      { false };
   bool                    writing             { false };
   bool                    closeRequested      { false };

   /// Public SseConnection handle associated with this transport state.
   http::SseConnectionPtr  ssePtr;
   std::weak_ptr<SseContext> context;
};
using SseStatePtr  = std::shared_ptr<SseState>;
using SseStateUPtr = std::unique_ptr<SseState>;

} // namespace sse

class SseConnection
{
public:
   using SendHandler  = std::function<void(std::string)>;
   using CloseHandler = std::function<void()>;

private:
   SendHandler       _sendHandler;
   CloseHandler      _closeHandler;
   std::atomic_bool  _closed { false };
   
   std::weak_ptr<Connection> _connection;
   std::any                  _userData;
   asio::ip::tcp::endpoint   _remoteEndpoint;
   std::string               _identifier;

public:
   SseConnection(const SseConnection&) = delete;
   SseConnection& operator=(const SseConnection&) = delete;
   ~SseConnection() = default;

   SseConnection(
      SendHandler sendHandler, 
      CloseHandler closeHandler,
      ConnectionPtr connection, 
      const asio::ip::tcp::endpoint& ep,
      const std::any& userData);

   /// Send one SSE event. Data containing newlines is emitted as multiple data fields.
   void send(std::string_view data, std::string_view event = {}, std::string_view id = {});

   /// Send an SSE comment, commonly used as a heartbeat.
   void comment(std::string_view text);

   /// Close the stream and send the terminating HTTP chunk.
   void close(const std::string& reason="");

   ConnectionId id() const;
   std::string identifier() const;
   asio::ip::tcp::endpoint remoteEndpoint() const;
   bool closed() const;
   std::any& userData();
   void identifier(const std::string& id);
};


class SseContext : public std::enable_shared_from_this<SseContext>
{
public:
   using OpenHandler  = std::function<void(SseConnectionPtr)>;
   using CloseHandler = std::function<void(SseConnectionPtr)>;

   OpenHandler onOpen;
   CloseHandler onClose;

   void add(const SseConnectionPtr& conn);

   void remove(const SseConnectionPtr& conn);

   void send(std::string_view data, std::string_view event, std::string_view id, const std::string& connIdentifier);

   void comment(std::string_view text, const std::string& connIdentifier);

   /// Close the specified connection and terminate underlying socket
   void close(ConnectionId connId, const std::string& reason="");

private:
   std::vector<SseConnectionPtr> connectionsSnapshot() const;

   mutable std::mutex _connectionsMutex;
   std::set<SseConnectionPtr> _connections;
};

} // namespace http
} // namespace tbs