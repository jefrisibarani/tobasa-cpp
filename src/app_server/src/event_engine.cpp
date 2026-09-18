#include <tobasa/util.h>
#include "event_engine.h"

namespace tbs {
namespace app {

EventEngine::~EventEngine()
{
}

EventEngine::EventEngine(app::DbServicePtr dbService)
   : _dbService {dbService}
{
   wsContext = std::make_shared<http::WebSocketContext>();
   sseContext = std::make_shared<http::SseContext>();

   sseContext->onOpen = [this](http::SseConnectionPtr conn)
   {
      auto user = getAuthenticatedAppUser(conn);
      if (user)
      {
         conn->identifier(user->uuid);

         Logger::logD("[webapp] [conn:{}] SSE connection started for user: {}", conn->id(), user->firstName);
         Json data;
         data["sseConnIdentity"] = conn->identifier();

         EventMessage message(
            "sse.connected",
            "SSE connection established",
            data);

         Json msg = message;
         conn->send(msg.dump(), message.type);
      }
      else
      {
         // Fatal error here. stop and close underlying connection
         sseContext->close(conn->id());
      }
   };

   sseContext->onClose = [this](http::SseConnectionPtr conn)
   {
      Logger::logD("[webapp] [conn:{}] sse connection closed", conn->id());
   };


   wsContext->onOpen = [this](http::WebSocketPtr conn)
   {
      auto user = getAuthenticatedAppUser(conn);
      if (user)
      {
         conn->identifier(user->uuid);

         Logger::logD("[webapp] [conn:{}] websocket connection started for user: {}", conn->id(), user->firstName);
         conn->sendText("Welcome " + user->firstName);

         Json data;
         data["wsConnIdentity"] = conn->identifier();

         EventMessage message(
            "websocket.connected",
            "WebSocket connection established",
            data);

         Json msg = message;
         conn->sendText(msg.dump());
      }
      else
      {
         // Fatal error here. stop and close underlying connection
         wsContext->close(conn->id(), "Invalid user", http::WS_CLOSE_CODE_PROTOCOL_ERROR);
      }
   };

   wsContext->onClose = [](http::WebSocketPtr conn, int closeCode, const std::string& reason)
   {
      Logger::logD("[webapp] [conn:{}] connection closed, code: {}, reason: {}", conn->id(), closeCode, reason);
   };

   wsContext->onPing = [](http::WebSocketPtr conn)
   {
      Logger::logD("[webapp] [conn:{}] received PING", conn->id());
   };

   wsContext->onPong = [](http::WebSocketPtr conn)
   {
      Logger::logD("[webapp] [conn:{}] received PONG", conn->id());
   };

   wsContext->onMessage = [this]
   (http::WebSocketPtr conn, const std::string& message)
   {
      Logger::logT("[webapp] [conn:{}] received data: {}", conn->id(), message);
      // -------------------------------------------------------

      // TODO_JEFRI:
      // check wether the connection still in a valid session
   };

   wsContext->onError = [this](http::WebSocketPtr conn, const http::ErrorData& error)
   {
      Logger::logT("[webapp] [conn:{}] error code: {}, {}", conn->id(), error.code, error.message);
   }; 

}

std::shared_ptr<http::WebSocketContext> EventEngine::appSocketContext()
{
   return wsContext;
}

std::shared_ptr<http::SseContext> EventEngine::appSseContext()
{
   return sseContext;
}

void EventEngine::sendMessage(const EventMessage& message, const std::string& connIdentifier)
{
   Json msg = message;
   wsContext->sendText(msg.dump(), connIdentifier);
}

void EventEngine::sendSseMessage(const EventMessage& message, const std::string& connIdentifier)
{
   Json msg = message;   
   sseContext->send(msg.dump(), message.type, "", connIdentifier);
}

web::entity::UserPtr EventEngine::getAuthenticatedAppUser(http::WebSocketPtr conn)
{
   if (conn == nullptr)
      return nullptr;

   auto& authResult = std::any_cast<web::AuthResult&>( conn->userData() );
   auto& identity = authResult.identity;

   return identity.pUser;
}

web::entity::UserPtr EventEngine::getAuthenticatedAppUser(http::SseConnectionPtr conn)
{
   if (conn == nullptr)
      return nullptr;

   auto& authResult = std::any_cast<web::AuthResult&>( conn->userData() );
   auto& identity = authResult.identity;

   return identity.pUser;
}

}} // namespace tbs::app   