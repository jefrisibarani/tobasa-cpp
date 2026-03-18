#include <sstream>
#include <tobasa/config.h>
#include <tobasaweb/settings_webapp.h>
#include <tobasahttp/websocket.h>
#include <tobasahttp/server/status_page.h>
#include <tobasa/format.h>
#include <tobasa/datetime.h>
#include <tobasa/util.h>
#include <tobasa/json.h>

#include "../page.h"
#include "../clock.h" // for websocket test
#include "websocket_controller.h"

#ifdef TOBASA_TEST_WSCHAT_USE_PROBUF
   #include "proto/chat_types_samples.pb.h"
   #include <google/protobuf/text_format.h>
#endif

namespace tbs {
namespace test {

using namespace http;

// For websocket test
//static tbs::Clock webClock(90); // every 90 second

WebsocketController::WebsocketController()
{
#ifdef TOBASA_TEST_WSCHAT_USE_PROBUF
  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;
#endif
}

WebsocketController::~WebsocketController()
{
   //webClock.stop();
}

void WebsocketController::bindHandler()
{
   using namespace std::placeholders;
   auto self(this);

   //! Handle GET request to /test_websocket
   router()->httpGet("/test_websocket", std::bind(&WebsocketController::onTestWebSocket, self, _1), AuthScheme::COOKIE);
   //! Handle WebSocket request to /websocket_ep ( ws://server/websocket_ep )
   router()->httpGet("/websocket_ep",   std::bind(&WebsocketController::onWebSocketA, self, _1),    AuthScheme::COOKIE);
   //! Handle WebSocket request to /websocket_ep_1  ( ws://server/websocket_ep_1 )
   router()->httpGet("/websocket_ep_1", std::bind(&WebsocketController::onWebSocketB, self, _1),    AuthScheme::COOKIE);

   _webSocketContext = std::make_shared<http::WebSocketContext>();

   _webSocketContext->onOpen = [this](WebSocketPtr conn)
   {
      Logger::logD("[websocket:{}] connection started", conn->id());

      // -------------------------------------------------------
      // add user to the list
      auto chatUserId = util::getRandomString(10);
      conn->identifier( chatUserId );
      _chatUsers.emplace(std::make_shared<ChatUser>( conn->id(), chatUserId) );

      // -------------------------------------------------------
      // Send welcome message
      std::ostringstream out;
      out <<  "Welcome to Tobasa Web Socket Service" << std::endl;
      sendMessage("APP_TEXT", out.str() , "server001", chatUserId, "TEXT", conn);

      // -------------------------------------------------------
      // Send user info to client
      Json userInfo;
      userInfo["userId"] = chatUserId;
      userInfo["connId"] = conn->id();
      sendMessage("SYS_RET_USER_INFO", userInfo.dump() , "server001", chatUserId, "SYS", conn);
   };

   _webSocketContext->onClose = [this](WebSocketPtr conn, int closeCode, const std::string& reason)
   {
      Logger::logD("[websocket:{}] connection closed, code: {}, reason: {}", conn->id(), closeCode, reason);
      
      // Remove user from our list
      for (auto user: _chatUsers)
      {
         if (user->connId == conn->id())
         {
            _chatUsers.erase(user);
            return;
         }
      }
   };

   _webSocketContext->onPing = [](WebSocketPtr conn)
   {
      Logger::logD("[websocket:{}] received PING", conn->id());
   };

   _webSocketContext->onPong = [](WebSocketPtr conn)
   {
      Logger::logD("[websocket:{}] received PONG", conn->id());
   };

   _webSocketContext->onMessage = [this](WebSocketPtr conn, const std::string& message)
   {
      Logger::logD("[websocket:{}] received data: {}", conn->id(), message);

      std::string timestamp;
      std::string messageId;
      std::string messageType;
      std::string senderId;
      std::string senderName;
      std::string receiverId;
      std::string content;
      std::string messageCmd;

#ifdef TOBASA_TEST_WSCHAT_USE_PROBUF
      chatmsg::ChatMessage msgIn;
      if (msgIn.ParseFromString(message))
      {
         timestamp   = msgIn.timestamp();
         messageId   = msgIn.id();
         messageType = msgIn.message_type();
         senderId    = msgIn.sender_id();
         senderName  = msgIn.sender_name();
         receiverId  = msgIn.receiver_id();
         content     = msgIn.content();
         messageCmd  = msgIn.message_cmd();
      }
      else
      {
         Logger::logE("[websocket:{}] invalid message syntax: {}", conn->id(), message);
         return;
      }
#else
      // Syntax: TBSMSG|{timestamp}|{message_id}|{message_type}|{sender_id}|{sender_name}|{receiver_id}|{messageCmd}|{content}
      // e.g   : TBSMSG|1738328617569|-OHwSQ8XFWvkgCqOFzKd|MSG_TYPE_SYS|server001|server001|VziZC9hcD1|SYS_RET_USER_LIST|[{"userId":"VziZC9hcD1","userName":"Smith"}]'
      // message_type : MSG_TYPE_TEXT, MSG_TYPE_STICKER, MSG_TYPE_NOTIFICATION, MSG_TYPE_SYS 
      // message_cmd  : APP_TEXT, SYS_KILL_ME_NOW, SYS_JOIN_CHAT, SYS_GET_USER_LIST, SYS_RET_USER_LIST, SYS_RET_USER_INFO
      auto items = util::split(message,"|");
      if (items.size() == 9)
      {
         timestamp   = items[1];
         messageId   = items[2];
         messageType = items[3];
         senderId    = items[4];
         senderName  = items[5];
         receiverId  = items[6];
         messageCmd  = items[7];
         content     = items[8];
      }
      else
      {
         Logger::logE("[websocket:{}] invalid message syntax: {}", conn->id(), message);
         return;
      }
#endif
      // Check for specific 'SYS message"
      if (messageCmd == "SYS_KILL_ME_NOW")
      {
         _webSocketContext->close(conn->id(), "Connection closure requested by client");
         return;
      }
      else if (messageCmd == "SYS_JOIN_CHAT")
      {
         // Add user to userlist
         auto user = getUser(conn->id());
         if (user != nullptr)
            user->userName = senderName;

         // Update all connected user userlist
         std::string userListJson    = getUserListAsJsonString();
         sendMessage("SYS_RET_USER_LIST", userListJson, "server001", "all_user", "SYS");
         return;
      }
      else if (messageCmd == "SYS_GET_USER_LIST")
      {
         std::string userListJson   = getUserListAsJsonString();
         // Send message to destination user
         sendMessage("SYS_RET_USER_LIST", userListJson , "server001", senderId, "SYS", conn);
         return;
      }
      else if ( !receiverId.empty() && receiverId == "all_user" )
      {
         // Send message to all user
         sendMessage("APP_TEXT", content , senderId, "all_user", "TEXT");
         return;
      }
      else if ( !receiverId.empty() && receiverId != "server001" )
      {
         // Send message to final user
         sendMessage("APP_TEXT", content , senderId, receiverId, "TEXT");
         return;
      }
      else
      {
         // Echo back to sender
         sendMessage("APP_TEXT", "[echo] " + content , "server001", senderId, "TEXT", conn);
         return;
      }
   };

   _webSocketContext->onError = [this](WebSocketPtr conn, const ErrorData& error)
   {
      Logger::logE("[websocket:{}] error code: {}, {}", conn->id(), error.code, error.message);
   };

   // Note: Disable broadcast test
   // webClock.start( [&]() {
   //    if (! _webSocketContext->hasClient() ) {
   //       return;
   //    }

   //    auto timestamp = currentUnixTimeStamp();
   //    auto content  = tbsfmt::format("[broadcast] {}", timestamp);
   //    sendMessage("APP_TEXT", content , "server001", "all_user", "TEXT");
   // });
}

//! Handle GET request to /test_websocket
http::ResultPtr WebsocketController::onTestWebSocket(const web::RouteArgument& arg)
{
   auto httpContext = arg.httpContext();
   try
   {
      auto page = std::make_shared<web::Page>(httpContext);
      page->title("Test Websocket - Tobasa Web Service");
      page->data("pageBodyClass", "");

#ifdef TOBASA_TEST_WSCHAT_USE_PROBUF
      page->data("dataTestProtobuf", true);
#else
      page->data("dataTestProtobuf", false);
#endif

      return page->show("test_websocket.tpl");
   }
   catch(const std::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] onTestWebSocket, {}", httpContext->connId(), ex.what());
      return statusResultHtml(StatusCode::INTERNAL_SERVER_ERROR);
   }
}

//! Handle WebSocket request to /websocket_ep
http::ResultPtr WebsocketController::onWebSocketA(const web::RouteArgument& arg)
{
   auto httpContext = arg.httpContext();
   // set a context and establish websocket connection.
   httpContext->webSocketContext(_webSocketContext);
   return http::makeResult();
}

//! Handle WebSocket request to /websocket_ep_1
http::ResultPtr WebsocketController::onWebSocketB(const web::RouteArgument& arg)
{
   auto httpContext = arg.httpContext();
   
   // we can assign another websocket handler,
   // but this sample we use the same handler

   // set a context and establish websocket connection.
   httpContext->webSocketContext(_webSocketContext);
   return http::makeResult();
}

std::string WebsocketController::currentUnixTimeStamp()
{
   auto now = DateTime::now();
   auto timestamp = std::to_string(now.toUnixTimeMiliSeconds());
   return timestamp;
}

WebsocketController::ChatUserPtr WebsocketController::getUser(http::ConnectionId connId)
{
   for (auto user: _chatUsers)
   {
      if (user->connId == connId)
         return user;
   }
   return nullptr;
}

WebsocketController::ChatUserPtr WebsocketController::getUser(const std::string& identifier)
{
   for (auto user: _chatUsers)
   {
      if (user->userId == identifier)
         return user;
   }
   return nullptr;
}

std::string WebsocketController::getUserListAsJsonString()
{
   // get all user as json array
   auto userList = Json::array();
   for (auto us: _chatUsers)
   {
      auto user = Json::object();
      user["userId"]   = us->userId;
      user["userName"] = us->userName;
      userList.emplace_back(user);
   }
   return userList.dump();
}

void WebsocketController::sendMessage(
            const std::string& messageCmd,
            const std::string& content,
            const std::string& senderId,
            const std::string& receiverId,
            const std::string& messageType,
            http::WebSocketPtr conn)
{
#ifdef TOBASA_TEST_WSCHAT_USE_PROBUF
   chatmsg::ChatMessageType msgType = chatmsg::ChatMessageType::MSG_TYPE_TEXT;
   if (messageType == "TEXT")
      msgType = chatmsg::ChatMessageType::MSG_TYPE_TEXT;
   if (messageType == "STICKER")
      msgType = chatmsg::ChatMessageType::MSG_TYPE_STICKER;
   if (messageType == "NOTIFICATION")
      msgType = chatmsg::ChatMessageType::MSG_TYPE_NOTIFICATION;
   if (messageType == "SYS")
      msgType = chatmsg::ChatMessageType::MSG_TYPE_SYS;

   std::string senderName;
   if (senderId == "server001")
      senderName = senderId;
   else
   {
      auto user = getUser(senderId);
      if (user != nullptr)
         senderName = user->userName;
   }

   if (receiverId == "all_user")
   {
      for (auto user: _chatUsers)
      {
         chatmsg::ChatMessage msgOut;
         msgOut.set_id(            util::generateUniqueId());
         msgOut.set_content(       content);
         msgOut.set_sender_id(     senderId);
         msgOut.set_sender_name(   senderName );
         msgOut.set_receiver_id(   user->userId);
         msgOut.set_timestamp(     currentUnixTimeStamp());
         msgOut.set_message_type(  msgType);
         msgOut.set_message_cmd(   messageCmd);

         std::string payload;
         if (msgOut.SerializeToString(&payload) )
            _webSocketContext->sendBinary( payload, user->userId ); 
      }
   }
   else 
   {
      chatmsg::ChatMessage msgOut;
      msgOut.set_id(            util::generateUniqueId());
      msgOut.set_content(       content);
      msgOut.set_sender_id(     senderId);
      msgOut.set_sender_name(   senderName );
      msgOut.set_receiver_id(   receiverId);
      msgOut.set_timestamp(     currentUnixTimeStamp());
      msgOut.set_message_type(  msgType);
      msgOut.set_message_cmd(   messageCmd);

      std::string payload;
      if (msgOut.SerializeToString(&payload) )
      {
         if (conn != nullptr)
         {
            // send back to the originating connection
            auto user = getUser(conn->id());
            if (user != nullptr && user->userId == receiverId)
               conn->sendBinary(payload);
            else 
            {
               throw std::runtime_error("invalid conn object for sender");
            }
         }
         else
            _webSocketContext->sendBinary( payload, receiverId ); 
      }
   }
#else

   // Syntax: TBSMSG|{timestamp}|{message_id}|{message_type}|{sender_id}|{sender_name}|{receiver_id}|{messageCmd}|{content}
   // e.g   : TBSMSG|1738328617569|-OHwSQ8XFWvkgCqOFzKd|MSG_TYPE_SYS|server001|server001|VziZC9hcD1|SYS_RET_USER_LIST|[{"userId":"VziZC9hcD1","userName":"Smith"}]'
   // message_type : MSG_TYPE_TEXT, MSG_TYPE_STICKER, MSG_TYPE_NOTIFICATION, MSG_TYPE_SYS 
   // message_cmd  : APP_TEXT, SYS_KILL_ME_NOW, SYS_JOIN_CHAT, SYS_GET_USER_LIST, SYS_RET_USER_LIST, SYS_RET_USER_INFO

   std::string senderName;
   if (senderId == "server001")
      senderName = senderId;
   else
   {
      auto user = getUser(senderId);
      if (user != nullptr)
         senderName = user->userName;
   }

   if (receiverId == "all_user")
   {
      for (auto user: _chatUsers)
      {
         std::ostringstream msg;
         msg << "TBSMSG" << "|" << currentUnixTimeStamp();
         msg << "|" << util::generateUniqueId();
         msg << "|" << "MSG_TYPE_" << messageType;
         msg << "|" << senderId << "|" << senderName;
         msg << "|" << user->userId;
         msg << "|" << messageCmd << "|" << content;

         _webSocketContext->sendText(msg.str(), user->userId); 
      }
   }
   else
   {
      std::ostringstream msg;
      msg << "TBSMSG" << "|" << currentUnixTimeStamp();
      msg << "|" << util::generateUniqueId();
      msg << "|" << "MSG_TYPE_" << messageType;
      msg << "|" << senderId << "|" << senderName;
      msg << "|" << receiverId;
      msg << "|" << messageCmd << "|" << content;

      if (conn != nullptr)
      {
         auto user = getUser(conn->id());
         if (user != nullptr && user->userId == receiverId)
            conn->sendText(msg.str());
         else 
            throw std::runtime_error("invalid conn object for sender");
      }
      else
         _webSocketContext->sendText( msg.str(), receiverId ); 
   }
#endif
}

} // namespace test
} // namespace tbs