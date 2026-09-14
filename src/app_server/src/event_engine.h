#pragma once

#include <string>
#include <utility>
#include <tobasahttp/websocket.h>
#include <tobasa/json.h>
#include <tobasaweb/credential_info.h>
#include "database_service_factory_app.h"

namespace tbs {
namespace app {

class EventMessage
{
public:
   std::string type;
   std::string message;
   Json data;

   EventMessage() = default;

   EventMessage(std::string eventType, std::string eventMessage, Json eventData = {})
      : type(std::move(eventType))
      , message(std::move(eventMessage))
      , data(std::move(eventData))
   {
   }

   Json toJson() const
   {
      Json result;
      result["type"] = type;
      result["message"] = message;
      if (!data.is_null() && !data.empty())
         result["data"] = data;
      return result;
   }

};

class EventEngine
{
public :
   EventEngine( const EventEngine & ) = delete;
   EventEngine( EventEngine && ) = delete;
   explicit EventEngine(app::DbServicePtr dbService);
   ~EventEngine();

   std::shared_ptr<http::WebSocketContext> appSocketContext();

   // Send message to connected client
   // Example - Send a notification:

   // Json notifAction;
   // notifAction["actionType"] = "button";
   // notifAction["actionLink"] = "/pacs/download/fafadfaffdfadfadsfadsf";

   // Json notifData;
   // notifData["type"]      = "success";
   // notifData["title"]     = "Export Completed";
   // notifData["content"]   = "Your Data File export is ready for download";
   // notifData["timestamp"] = DateTime::now().toUnixTimeMiliSeconds();
   // notifData["action"]    = notifAction;

   // EventMessage notification("notification", "Export completed", notifData);
   // eventEngine->sendMessage(notification, userIdentifier);
   
   void sendMessage(const EventMessage& message, const std::string& wsConnIdentity);

private:
   web::entity::UserPtr getAuthenticatedAppUser(http::WebSocketPtr conn);

private:
   app::DbServicePtr _dbService {nullptr};
   std::shared_ptr<http::WebSocketContext> wsContext;
};

}} // namespace tbs::app