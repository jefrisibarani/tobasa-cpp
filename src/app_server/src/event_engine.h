#pragma once

#include <string>
#include <utility>
#include <tobasahttp/websocket.h>
#include <tobasahttp/sse.h>
#include <tobasa/json.h>
#include <tobasaweb/credential_info.h>
#include "database_service_factory_app.h"

namespace tbs {
namespace app {

struct EventAction
{
   std::string type;
   std::string link;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EventAction, type, link)

struct EventData
{
   std::string type;
   std::string title;
   std::string content;
   long long   timestamp;
   Json        action;   // EventAction
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EventData, type, title, content, timestamp, action)

class EventMessage
{
public:
   std::string type;
   std::string message;
   Json        data;    // Event Data

   EventMessage() = default;

   EventMessage(std::string type_, std::string message_, Json data_ = {})
      : type (std::move(type_))
      , message (std::move(message_))
      , data (std::move(data_))
   {
   }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EventMessage, type, message, data)

class EventEngine
{
public :
   EventEngine( const EventEngine & ) = delete;
   EventEngine( EventEngine && ) = delete;
   explicit EventEngine(app::DbServicePtr dbService);
   ~EventEngine();

   std::shared_ptr<http::WebSocketContext> appSocketContext();
   std::shared_ptr<http::SseContext> appSseContext();

   // Send message to connected client
   // Example - Send a notification:

   // EventAction action;
   // action.type = "button";
   // action.link = "/pacs/download/fafadfaffdfadfadsfadsf";

   // EventData data;
   // data.type      = "success";
   // data.title     = "Password changed successfully";
   // data.content   = "Your User Data File export is ready for download";
   // data.timestamp = DateTime::now().toUnixTimeMiliSeconds();
   // data.action    = action;

   // EventMessage message("notification", "Password change completed", data);
   // eventEngine->sendMessage(message, userIdentifier);
   
   void sendMessage(const EventMessage& message, const std::string& connIdentifier);
   
   void sendSseMessage(const EventMessage& message, const std::string& connIdentifier);

private:
   web::entity::UserPtr getAuthenticatedAppUser(http::WebSocketPtr conn);
   web::entity::UserPtr getAuthenticatedAppUser(http::SseConnectionPtr conn);

private:
   app::DbServicePtr _dbService {nullptr};
   std::shared_ptr<http::WebSocketContext> wsContext;
   std::shared_ptr<http::SseContext> sseContext;
};

}} // namespace tbs::app