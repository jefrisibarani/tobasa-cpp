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

void WebSocketState::configureTransport(WebSocketTransport value)
{
   transport = std::move(value);
}

void WebSocketState::sendFromQueue()
{
   if (transport.closed && transport.closed())
   {
      ErrorData error;
      error.message = "Connection already closed";
      handleConnError(error);
      onError(error);
      return;
   }

   if (sendQueue.empty() || !transport.write)
      return;

   if (transport.startWriteTimer)
      transport.startWriteTimer();

   transport.write(sendQueue.front(),
      [this](const std::error_code& error, std::size_t bytesTransferred)
      {
         if (transport.cancelTimer)
            transport.cancelTimer();

         if (!error)
         {
            ws::SendErrorHandler callback;
            
            auto it = sendQueue.begin();
            if (it != sendQueue.end())
            {
               callback = std::move(it->callback);
               sendQueue.erase(it);
            }

            const bool releaseState = closed && sendQueue.empty();
            if (!releaseState && !sendQueue.empty())
               sendFromQueue();

            if (callback)
            {
               ErrorData err;
               err.code = error.value();
               err.message = error.message();
               callback(err);
            }
         }
         else
         {
            ErrorData err;
            err.code = error.value();
            err.message = error.message();
            
            handleConnError(err, bytesTransferred);
            onError(err);

            if (transport.error)
               transport.error(error, ErrorType::system);
         }
      });
}

void WebSocketState::send(const std::shared_ptr<OutMessage>& outMessage, ws::SendErrorHandler callback, unsigned char finRsvOpcode)
{
   if (transport.closed && transport.closed())
      return;

   const std::size_t length = outMessage->size();
   auto outHeader = std::make_shared<OutMessage>(10);
   outHeader->put(static_cast<char>(finRsvOpcode));
   // Unmasked (first length byte<128)
   if (length >= 126)
   {
      std::size_t numBytes;
      if(length > 0xffff)
      {
         numBytes = 8;
         outHeader->put(127);
      }
      else
      {
         numBytes = 2;
         outHeader->put(126);
      }

      for(std::size_t c = numBytes - 1; c != static_cast<std::size_t>(-1); c--)
         outHeader->put((static_cast<uint64_t>(length) >> (8 * c)) % 256);
   }
   else
      outHeader->put(static_cast<char>(length));

   sendQueue.emplace_back(std::move(outHeader), std::move(outMessage), std::move(callback));
   if (sendQueue.size() == 1)
      sendFromQueue();
}

void WebSocketState::send(std::string_view outMessageStr, ws::SendErrorHandler callback, unsigned char finRsvOpcode)
{
   if (transport.closed && transport.closed())
      return;

   auto outMessage = std::make_shared<OutMessage>();
   outMessage->write(outMessageStr.data(), static_cast<std::streamsize>(outMessageStr.size()));
   send(outMessage, std::move(callback), finRsvOpcode);
}

void WebSocketState::sendClose(int32_t status, const std::string& reason, ws::SendErrorHandler callback)
{
   // Send close only once (in case close is initiated by server)

   if (closed)
      return;

   closed = true;
   
   auto sendStream = std::make_shared<OutMessage>();
   sendStream->put(status >> 8);
   sendStream->put(status % 256);
   *sendStream << reason;

   // finRsvOpcode=136: message close
   send(std::move(sendStream), std::move(callback), static_cast<unsigned char>(136));
}

void WebSocketState::sendBinary(const std::string& data, ws::SendErrorHandler callback)
{
   send(std::string_view(data), std::move(callback), static_cast<unsigned char>(130));
}

void WebSocketState::sendText(const std::string& data, ws::SendErrorHandler callback)
{
   send(std::string_view(data), std::move(callback), static_cast<unsigned char>(129));
}

void WebSocketState::readWebSocket()
{
   if (!transport.read)
      return;

   if (transport.startReadTimer)
      transport.startReadTimer();

   transport.read(2,
      [this](const std::error_code& error, std::size_t bytesTransferred)
      {
         if (transport.cancelTimer)
            transport.cancelTimer();

         if (error)
         {
            if (wsContext)
               onError(error, ErrorType::system);
            if (transport.error)
               transport.error(error, ErrorType::system);

            return;
         }

         if (bytesTransferred == 0)
         {
            readWebSocket();
            return;
         }

         // Read the first 2 header bytes
         std::istream istream(&sendStreamBuf);
         std::array<unsigned char, 2> firstBytes;
         istream.read(reinterpret_cast<char*>(&firstBytes[0]), 2);
         const unsigned char finRsvOpcode = firstBytes[0];

         // Close connection if unmasked message from client (protocol error)
         if (firstBytes[1] < 128)
         {
            const std::string reason("message from client not masked");
            sendClose(WS_CLOSE_CODE_PROTOCOL_ERROR, reason);
            onClose(    WS_CLOSE_CODE_PROTOCOL_ERROR, reason);

            if (transport.complete)
               transport.complete("websocket, " + reason);
            
            return;
         }

         const std::size_t length = firstBytes[1] & 127;

         if (length == 126 || length == 127)
         {
            // length is 126, two next bytes is the size of content
            // lenght is 127, eight next bytes is the size of content
            const std::size_t lengthBytes = (length == 126) ? 2 : 8;

            if (transport.startReadTimer)
               transport.startReadTimer();

            transport.read(lengthBytes,
               [this, finRsvOpcode, lengthBytes](const std::error_code& readError, std::size_t)
               {
                  if (transport.cancelTimer)
                     transport.cancelTimer();

                  if (readError)
                  {
                     if (wsContext)
                        onError(readError, ErrorType::system);

                     if (transport.error)
                        transport.error(readError, ErrorType::system);

                     return;
                  }

                  std::istream lengthStream(&sendStreamBuf);

                  std::array<unsigned char, 8> bytes{};
                  lengthStream.read(reinterpret_cast<char*>(&bytes[0]), static_cast<std::streamsize>(lengthBytes));
                  
                  std::size_t messageLength = 0;
                  // Decode the extended payload length as big-endian network bytes.
                  for (std::size_t index = 0; index < lengthBytes; ++index)
                  {
                     messageLength = (messageLength << 8) | bytes[index];
                  }
                  //for (std::size_t index = 0; index < lengthBytes; ++index)
                  //{
                  //   messageLength += static_cast<std::size_t>(bytes[index]) << (8 * (lengthBytes - 1 - index));
                  //}

                  readMessageContent(messageLength, finRsvOpcode);
               });
            return;
         }

         readMessageContent(length, finRsvOpcode);
      });
}

void WebSocketState::readMessageContent(std::size_t length, unsigned char finRsvOpcode)
{
   if (transport.messageMaxSize > 0 && length + (fragmentedInMessage ? fragmentedInMessage->length : 0) > transport.messageMaxSize)
   {
      const int32_t status = WS_CLOSE_CODE_MESSAGE_TOO_BIG;
      const std::string reason = "message too big";

      sendClose(status, reason);
      onClose(status, reason);
      
      if (transport.complete)
         transport.complete("websocket, " + reason);

      return;
   }

   if (transport.startReadTimer)
      transport.startReadTimer();

   transport.read(4 + length,
      [this, length, finRsvOpcode](const std::error_code& error, std::size_t)
      {
         if (transport.cancelTimer)
            transport.cancelTimer();
         
         if (error)
         {
            if (wsContext)
               onError(error, ErrorType::system);

            if (transport.error)
               transport.error(error, ErrorType::system);
            
            return;
         }

         std::istream istream(&sendStreamBuf);
         // Read mask
         std::array<unsigned char, 4> mask;
         istream.read(reinterpret_cast<char*>(&mask[0]), 4);

         InMessagePtr inMessage;

         // If fragmented message
         if ((finRsvOpcode & 0x80) == 0 || (finRsvOpcode & 0x0f) == 0)
         {
            if (!fragmentedInMessage)
            {
               fragmentedInMessage = std::make_shared<InMessage>(finRsvOpcode, length);
               fragmentedInMessage->finRsvOpcode |= 0x80;
            }
            else {
               fragmentedInMessage->length += length;
            }

            inMessage = fragmentedInMessage;
         }
         else {
            inMessage = std::make_shared<InMessage>(finRsvOpcode, length);
         }

         std::ostream ostream(&inMessage->streambuf);
         for (std::size_t index = 0; index < length; ++index)
         {
            ostream.put(istream.get() ^ mask[index % 4]);
         }

         unsigned char opcode = finRsvOpcode & 0x0f;

         // Connection close
         if (opcode == 8)
         {
            // Read the first two payload bytes from the incoming message stream 
            // and rebuilds a 16-bit status code in big-endian order
            int32_t status = 0;
            if (length >= 2)
            {
               unsigned char byte1 = inMessage->get();
               unsigned char byte2 = inMessage->get();
               status = (static_cast<int32_t>(byte1) << 8) + byte2;
            }

            auto reason = inMessage->string();
            sendClose(status, reason);
            onClose(status, reason);

            if (transport.complete)
               transport.complete("websocket, " + reason);
         }
         // Ping
         else if (opcode == 9)
         {
            // Send Pong
            auto outMessage = std::make_shared<OutMessage>();
            *outMessage << inMessage->string();
            send(outMessage, nullptr, finRsvOpcode + 1);

            onPing();

            readWebSocket();  // Next message
         }
         // Pong
         else if (opcode == 10)
         {
            onPong();

            readWebSocket();  // Next message
         }
         // Fragmented message and not final fragment
         else if ((finRsvOpcode & 0x80) == 0)
         {
            readWebSocket(); // Next message
         }
         else
         {
            onMessage(inMessage->string());
            
            // Only reset _wsFragmentedInMessage for non-control frames 
            // (control frames can be in between a fragmented message)
            fragmentedInMessage = nullptr;
            
            readWebSocket();  // Next message
         }
      });
}

void WebSocketState::handleConnError(const ErrorData& error, std::size_t)
{
   // All handlers in the queue is called with error
   std::vector<ws::SendErrorHandler> callbacks;
   for (auto& outData : sendQueue)
   {
      if (outData.callback)
         callbacks.emplace_back(std::move(outData.callback));
   }

   sendQueue.clear();

   for (auto& callback : callbacks)
      callback(error);
}


#ifdef TOBASA_HTTP_USE_HTTP2
void WebSocketState::feedHttp2WebSocket(const uint8_t* data, std::size_t length)
{
   receiveBuffer.insert(receiveBuffer.end(), data, data + length);

   while (receiveBuffer.size() >= 2 && !closed)
   {
      const uint8_t  first          = receiveBuffer[0];
      const uint8_t  second         = receiveBuffer[1];
      const bool     fin            = (first & 0x80) != 0;
      const uint8_t  opcode         = first & 0x0f;
      const bool     masked         = (second & 0x80) != 0;
      uint64_t       payloadLength  = second & 0x7f;
      std::size_t    headerLength   = 2;

      if (!masked)
      {
         const std::string reason = "message from client not masked";
         sendClose(WS_CLOSE_CODE_PROTOCOL_ERROR, reason);
         onClose(    WS_CLOSE_CODE_PROTOCOL_ERROR, reason);

         if (transport.complete)
            transport.complete("websocket, " + reason);

         return;
      }

      if (payloadLength == 126)
      {
         if (receiveBuffer.size() < 4)
            return;

         payloadLength = (static_cast<uint64_t>(receiveBuffer[2]) << 8) | receiveBuffer[3];
         headerLength = 4;
      }
      else if (payloadLength == 127)
      {
         if (receiveBuffer.size() < 10)
            return;
         
         payloadLength = 0;
         for (std::size_t index = 2; index < 10; ++index)
            payloadLength = (payloadLength << 8) | receiveBuffer[index];

         headerLength = 10;
      }

      const bool control = opcode >= 8;
      if ((control && (!fin || payloadLength > 125)) ||
          (transport.messageMaxSize > 0 && payloadLength > transport.messageMaxSize))
      {
         const std::string reason = "invalid WebSocket frame";
         sendClose(WS_CLOSE_CODE_PROTOCOL_ERROR, reason);
         onClose(    WS_CLOSE_CODE_PROTOCOL_ERROR, reason);

         if (transport.complete)
            transport.complete("websocket, " + reason);
         
         return;
      }

      const std::size_t frameLength = headerLength + 4 + static_cast<std::size_t>(payloadLength);
      if (receiveBuffer.size() < frameLength)
         return;

      const uint8_t* mask = receiveBuffer.data() + headerLength;
      std::string payload(static_cast<std::size_t>(payloadLength), '\0');
      for (std::size_t index = 0; index < payload.size(); ++index)
         payload[index] = static_cast<char>(receiveBuffer[headerLength + 4 + index] ^ mask[index % 4]);

      receiveBuffer.erase(receiveBuffer.begin(), receiveBuffer.begin() + frameLength);

      // Connection Close
      if (opcode == 8)
      {
         if (payload.size() == 1)
         {
            const std::string reason = "invalid close frame";
            sendClose(WS_CLOSE_CODE_PROTOCOL_ERROR, reason);
            onClose(    WS_CLOSE_CODE_PROTOCOL_ERROR, reason);

            if (transport.complete)
               transport.complete("websocket, " + reason);

            return;
         }

         int32_t status = WS_CLOSE_CODE_NORMAL_CLOSURE;
         std::string reason;
         if (payload.size() >= 2)
         {
            status = (static_cast<uint8_t>(payload[0]) << 8) | static_cast<uint8_t>(payload[1]);
            reason = payload.substr(2);
         }

         sendClose(status, reason);
         onClose(status, reason);
         
         if (transport.complete)
            transport.complete("websocket, " + reason);
         
         return;
      }
      // Ping
      if (opcode == 9)
      {
         send(payload, nullptr, 0x8a);
         onPing();
         continue;
      }
      // Pong
      if (opcode == 10)
      {
         onPong();
         continue;
      }

      if (!fin || opcode == 0)
      {
         if (opcode != 0 && !fragmentedInMessage)
         {
            fragmentedInMessage = std::make_shared<InMessage>(opcode, payload.size());
            fragmentedInMessage->finRsvOpcode |= 0x80;
         }
         else if (opcode == 0 && !fragmentedInMessage)
         {
            const std::string reason = "unexpected continuation frame";
            sendClose(WS_CLOSE_CODE_PROTOCOL_ERROR, reason);
            onClose(    WS_CLOSE_CODE_PROTOCOL_ERROR, reason);
            
            if (transport.complete)
               transport.complete("websocket, " + reason);

            return;
         }

         if (fragmentedInMessage)
         {
            std::ostream output(&fragmentedInMessage->streambuf);
            output << payload;
            fragmentedInMessage->length += payload.size();
            if (fin)
            {
               onMessage(fragmentedInMessage->string());
               fragmentedInMessage.reset();
            }
         }
      }
      else
         onMessage(payload);
   }
}
#endif

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
      ConnectionId id,
      const asio::ip::tcp::endpoint& ep, 
      const Headers& requestHeader,
      const std::any& userData )
   : _connection {conn}
   , _connId {id}
   , _remoteEndpoint {ep}
   , _requestHeaders {requestHeader}
   , _userData {userData}
{}

void WebSocket::sendText(const std::string& data, ws::SendErrorHandler callback)
{
   if (_sendTextHandler)
   {
      _sendTextHandler(data, std::move(callback));
      return;
   }

   auto connection = _connection.lock();
   if (!connection || connection->closed())
   {
      Logger::logT("[websocket] Connection already closed");
      return;
   }

   Logger::logT("[websocket] Connection {} does not have a WebSocket transport", connection->id());
}

void WebSocket::sendBinary(const std::string& data, ws::SendErrorHandler callback)
{
   if (_sendBinaryHandler)
   {
      _sendBinaryHandler(data, std::move(callback));
      return;
   }

   auto connection = _connection.lock();
   if (!connection || connection->closed())
   {
      Logger::logT("[websocket] Connection already closed");
      return;
   }

   Logger::logT("[websocket] Connection {} does not have a WebSocket transport", connection->id());
}

void WebSocket::close(const std::string& reason, int32_t closeCode)
{
   if (_sendCloseHandler)
   {
      _sendCloseHandler(closeCode, reason, nullptr);
      return;
   }

   auto connection = _connection.lock();
   if (!connection)
      return;

   Logger::logT("[websocket] Connection {} does not have a WebSocket transport", connection->id());

   // Use callClose() to ensure the ConnectionManager properly handles
   // the closure while the underlying connection is still available.
   connection->callClose(reason);
}

void WebSocket::setTransport(
   std::function<void(const std::string&, ws::SendErrorHandler)> sendText,
   std::function<void(const std::string&, ws::SendErrorHandler)> sendBinary,
   std::function<void(int32_t, const std::string&, ws::SendErrorHandler)> sendClose)
{
   _sendTextHandler   = std::move(sendText);
   _sendBinaryHandler = std::move(sendBinary);
   _sendCloseHandler  = std::move(sendClose);
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

void WebSocketContext::sendText(const std::string& data, ConnectionId connId, ws::SendErrorHandler callback)
{
   for (auto conn: connectionsSnapshot())
   {
      if (connId==0)
         conn->sendText(data, callback);
      else if (conn->id() == connId)
         conn->sendText(data, callback);
   }
}

void WebSocketContext::sendBinary(const std::string& data, ConnectionId connId, ws::SendErrorHandler callback)
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
      ws::SendErrorHandler callback, ws::SkipSendHandler skipCallback )
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
      ws::SendErrorHandler callback, ws::SkipSendHandler skipCallback )
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