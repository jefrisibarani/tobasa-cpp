#pragma once
#include <tobasaweb/controller_base.h>
#include <tobasaweb/router.h>

namespace tbs {
namespace test {

class WebsocketController
   : public web::ControllerBase
{
public :
   WebsocketController( const WebsocketController & ) = delete;
   WebsocketController( WebsocketController && ) = delete;

   explicit WebsocketController();

   ~WebsocketController();

   //! Handle GET request to /test_websocket
   http::ResultPtr onTestWebSocket(const web::RouteArgument& arg);

   //! Handle WebSocket request to /websocket_ep ( ws://server/websocket_ep )
   http::ResultPtr onWebSocketA(const web::RouteArgument& arg);

   //! Handle WebSocket request to /websocket_ep_1 ( ws://server/websocket_ep_1 )
   http::ResultPtr onWebSocketB(const web::RouteArgument& arg);

protected:
   void bindHandler();
   std::string currentUnixTimeStamp();
   
   // WebSocket context for our simple chat server
   std::shared_ptr<http::WebSocketContext> _webSocketContext;
   // Chat user
   struct ChatUser
   {
      ChatUser(http::ConnectionId cId, const std::string& uId)
         :connId {cId} , userId {uId} {}

      http::ConnectionId connId;
      std::string userId;
      std::string userName;
   };
   using ChatUserPtr = std::shared_ptr<ChatUser>;
   std::set<ChatUserPtr> _chatUsers;

   ChatUserPtr getUser(http::ConnectionId connId);
   ChatUserPtr getUser(const std::string& identifier);
   std::string getUserListAsJsonString();

   
   void sendMessage(
      const std::string& messageCmd, 
      const std::string& content, 
      const std::string& senderId, 
      const std::string& receiverId, 
      const std::string& messageType, 
      http::WebSocketPtr conn=nullptr );
};

} // namespace test
} // namespace tbs