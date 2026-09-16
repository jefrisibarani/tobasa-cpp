/*
   Adapted from Simple-WebSocket-Server by Christian Eidheim
   https://gitlab.com/eidheim/Simple-WebSocket-Server
*/

#pragma once

#include <iostream>
#include <vector>
#include <set>
#include <list>
#include <mutex>
#include <string_view>
#include <optional>
#include <asio/ip/tcp.hpp>
#include <asio/buffers_iterator.hpp>
#include <asio/basic_streambuf.hpp>
#include <asio/streambuf.hpp>
#include "tobasahttp/headers.h"
#include "tobasahttp/field.h"
#include "tobasahttp/connection.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/// Web Socket Close code

constexpr int32_t WS_CLOSE_CODE_NORMAL_CLOSURE        = 1000;
constexpr int32_t WS_CLOSE_CODE_GOING_AWAY            = 1001;
constexpr int32_t WS_CLOSE_CODE_PROTOCOL_ERROR        = 1002;
constexpr int32_t WS_CLOSE_CODE_UNSUPPORTED_DATA      = 1003;
constexpr int32_t WS_CLOSE_CODE_RESERVED              = 1004;
constexpr int32_t WS_CLOSE_CODE_NO_STATUS_RCVD        = 1005;
constexpr int32_t WS_CLOSE_CODE_ABNORMAL_CLOSURE      = 1006;
constexpr int32_t WS_CLOSE_CODE_INVALID_FRAME_PAYLOAD = 1007;
constexpr int32_t WS_CLOSE_CODE_POLICY_VIOLATION      = 1008;
constexpr int32_t WS_CLOSE_CODE_MESSAGE_TOO_BIG       = 1009;
constexpr int32_t WS_CLOSE_CODE_MANDATORY_EXTENSION   = 1010;
constexpr int32_t WS_CLOSE_CODE_INTERNAL_ERROR        = 1011;
constexpr int32_t WS_CLOSE_CODE_TLS_FAILURE           = 1015;

/**
 * @brief Interface for sending WebSocket frames.
 * Implemented by connection types that support WebSocket communication.
 */
class WebSocketSender
{
public:
   virtual ~WebSocketSender() = default;

   virtual void wsSendBinary(const std::string& data, WsSendErrorHandler callback = nullptr) = 0;
   virtual void wsSendText(const std::string& data, WsSendErrorHandler callback = nullptr) = 0;
   virtual void wsSendClose(int32_t status, const std::string &reason = "", WsSendErrorHandler callback = nullptr) = 0;
};

namespace ws {

/**
 * @brief Represents an incoming WebSocket message.
 */
class InMessage: public std::istream
{
public:
   unsigned char finRsvOpcode;
   std::size_t size() noexcept
   {
      return length;
   }

   std::string string() noexcept
   {
      return std::string(asio::buffers_begin(streambuf.data()), asio::buffers_end(streambuf.data()));
   }

   InMessage() noexcept
      : std::istream(&streambuf), length(0)
   {}

   InMessage(unsigned char finRsvOpcode, std::size_t length) noexcept
      : std::istream(&streambuf), finRsvOpcode(finRsvOpcode), length(length)
   {}

   std::size_t length;
   asio::streambuf streambuf;
};
using InMessagePtr = std::shared_ptr<InMessage>;

/**
 * @brief Outgoing WebSocket message buffer.
 * The buffer is not consumed while sending. Do not modify it until sending
 * is complete.
 */
class OutMessage: public std::ostream
{
public:
   asio::streambuf streambuf;

   OutMessage() noexcept: std::ostream(&streambuf)
   {}

   OutMessage(std::size_t capacity) noexcept
      : std::ostream(&streambuf)
   {
      streambuf.prepare(capacity);
   }

   /// Returns the size of the buffer
   std::size_t size() const noexcept
   {
      return streambuf.size();
   }
};

/**
 * @brief Queued outgoing WebSocket frame data and its send callback.
 */
class OutData
{
   public:
      OutData(std::shared_ptr<OutMessage> outHdr,
              std::shared_ptr<OutMessage> outMsg,
              WsSendErrorHandler &&cb) noexcept
         : outHeader(std::move(outHdr))
         , outMessage(std::move(outMsg))
         , callback(std::move(cb))
      {}

      std::shared_ptr<OutMessage> outHeader;
      std::shared_ptr<OutMessage> outMessage;
      WsSendErrorHandler callback;
};

/**
 * @brief WebSocket transport state used internally by ServerConnection.
 *
 * Created when an HTTP connection is upgraded to WebSocket. Owns the
 * WebSocket frame buffers, fragmentation state, outgoing queue, and event
 * dispatch context. The public WebSocket handle is passed to application
 * callbacks and stored by WebSocketContext.
 */
struct WebSocketState
{
   asio::streambuf     sendStreamBuf;
   bool                closed { false };
   InMessagePtr        fragmentedInMessage;
   std::list<OutData>  sendQueue;
   WebSocketContextPtr wsContext;

   /// Public WebSocket handle associated with this transport state.
   http::WebSocketPtr  wsPtr;

   void onOpen();
   void onClose(int32_t status, const std::string& reason);
   void onMessage(const std::string& message);
   void onError(const ErrorData& error);
   void onError(const std::error_code& error, ErrorType errorTpe, const std::string& source="WebSocketState");
   void onPing();
   void onPong();
};
using WebSocketStatePtr  = std::shared_ptr<WebSocketState>;
using WebSocketStateUPtr = std::unique_ptr<WebSocketState>;

using OnOpenHandler     = std::function<void(http::WebSocketPtr)>;
using OnCloseHandler    = std::function<void(http::WebSocketPtr, int32_t, const std::string&)>;
using OnPingHandler     = std::function<void(http::WebSocketPtr)>;
using OnPongHandler     = std::function<void(http::WebSocketPtr)>;
using OnMessageHandler  = std::function<void(http::WebSocketPtr, const std::string&)>;
using OnErrorHandler    = std::function<void(http::WebSocketPtr, const ErrorData&)>;

using SkipSendHandler   = std::function<bool(ConnectionId)>;
} // namespace ws

/**
 * @brief Public handle for an upgraded WebSocket connection.
 *
 * Provides application code with access to WebSocket messaging, connection
 * metadata, and connection control.
 */
class WebSocket
{
private:
   std::weak_ptr<Connection> _connection;  // HttpConnection
   asio::ip::tcp::endpoint   _remoteEndpoint;
   std::any                  _userData;
   std::string               _identifier;
   Headers                   _requestHeaders;

public:
   WebSocket(const WebSocket&) = delete;
   WebSocket& operator=(const WebSocket&) = delete;
   ~WebSocket() = default;

   WebSocket(ConnectionPtr conn, 
      const std::any& userData, 
      const asio::ip::tcp::endpoint& ep, 
      Headers& requestHeader);

   void sendText(  const std::string& data, WsSendErrorHandler callback = nullptr);
   void sendBinary(const std::string& data, WsSendErrorHandler callback = nullptr);

   /// Closes internal HTTP connection
   void close(const std::string& reason="", int32_t closeCode=WS_CLOSE_CODE_NORMAL_CLOSURE);
   
   ConnectionId id() const;
   std::string identifier() const;
   asio::ip::tcp::endpoint remoteEndpoint() const;
   bool closed() const;
   void identifier(const std::string& id);

   std::any& userData();
   Headers& requestHeaders();
};

class Context;

/** 
 * WebSocketContext
 * Manages all WebSocket connections
 * To accept WebSocket connections, Endpoint handler must set response status 200
 * and assign a WebSocketContext to the HttpContext
 * 
 * example:
 * @code
 *  WS endpoint is wss://localhost:8085/websocket_ep
 * 
 *  wsContext = std::make_shared<http::WebSocketContext>();
 *  // setup all handlers
 *  wsContext->onOpen = {...}
 * 
 *  http::Server server(...);
 *  // set server request handler
 *  serverHttp.requestHandler(
 *     [&](const http::HttpContext& context)
 *     {
 *         if (context->request()->path() == "/websocket_ep")
 *         {
 *            context->webSocketContext(wsContext);
 * 
 *            // Note: by default response has empty content and status 200
 *            // Here, we only need to give Http status 200
 *            response->content(""); 
 *            response->httpStatus( http::StatusCode::OK);
 *            
 *            return http::RequestStatus::handled;
 *         }
 *         else
 *         { ... } // handle other requests
 *     });
 * @endcode
 */
class WebSocketContext : public std::enable_shared_from_this<WebSocketContext>
{
   friend class http::Context;

private:
   std::vector<WebSocketPtr> connectionsSnapshot() const;

   mutable std::mutex     _connectionsMutex;
   std::set<WebSocketPtr> _connections;

public:
   WebSocketContext() = default;
   virtual ~WebSocketContext();

   // WebSocket handlers, set by user

   ws::OnOpenHandler    onOpen;
   ws::OnCloseHandler   onClose;
   ws::OnMessageHandler onMessage;
   ws::OnErrorHandler   onError;
   ws::OnPingHandler    onPing;
   ws::OnPongHandler    onPong;

   /// Add the specified connection to the manager and start it.
   void addConnection(WebSocketPtr conn);

   /// Stop the specified connection.
   void stop(WebSocketPtr conn, const std::string& reason = "");
   void stop(ConnectionId id, const std::string& reason = "");

   /// @brief Close the specified connection and terminate underlying socket
   void close(ConnectionId id, const std::string& reason = "", int32_t closeCode=WS_CLOSE_CODE_NORMAL_CLOSURE);

   bool isClientConnected(const std::string& identifier) const;

   WebSocketPtr findClient(const std::string& identifier) const;

   bool hasClient() const;

   /** 
    * Send data to client
    * @param data   Data to send to client(s)
    * @param connId Connection id, if none given, send data to all clients
    */
   void sendText(  const std::string& data, ConnectionId connId=0, WsSendErrorHandler callback = nullptr);
   
   void sendBinary(const std::string& data, ConnectionId connId=0, WsSendErrorHandler callback = nullptr);

   void sendText(const std::string& data, const std::string& identifier, 
      WsSendErrorHandler callback = nullptr, ws::SkipSendHandler skipCallback = nullptr);
      
   void sendBinary(const std::string& data, const std::string& identifier, 
      WsSendErrorHandler callback = nullptr, ws::SkipSendHandler skipCallback = nullptr);
};

/** @}*/

} // namespace http
} // namespace tbs