#include <tobasa/json.h>
#include "tobasaweb/alert.h"
#include "tobasaweb/session.h"

namespace tbs {
namespace web {

AlertPtr Alert::create(const std::string& sessionId)
{
   return std::make_shared<Alert>(sessionId);
}

Alert::Alert(const std::string& sessionId)
{
   _sessionId = sessionId;
}   

void Alert::success(const std::string& message, const std::string& location, bool autoClose)
{
   add(Alert::TYP_SUCCESS, message, location, autoClose);
}

void Alert::info(const std::string& message, const std::string& location, bool autoClose)
{
   add(Alert::TYP_INFO, message, location, autoClose);
}

void Alert::warning(const std::string& message, const std::string& location, bool autoClose)
{
   add(Alert::TYP_WARNING, message, location, autoClose);
}

void Alert::error(const std::string& message, const std::string& location, bool autoClose)
{
   add(Alert::TYP_ERROR, message, location, autoClose);
}

void Alert::add(const std::string& type, const std::string& message, const std::string& location, bool autoClose)
{
   Json alert;
   alert["type"]      = type;
   alert["message"]   = message;
   alert["location"]  = location;
   alert["id"]        = "";
   alert["autoClose"] = autoClose;

   auto ses = Session::get(_sessionId);
   Json alerts = ses->getAlerts();
   alerts.push_back(alert);

   ses->setAlerts(alerts);
}

bool Alert::isEmpty()
{
   auto ses = Session::get(_sessionId);
   Json alerts = ses->getAlerts();

   if (alerts.empty())
      return true;
   else
      return false;
}	

void Alert::removeAll()
{
   auto ses = Session::get(_sessionId);
   ses->setAlerts(Json::array());
}


} // namespace web
} // namespace tbs