#pragma once

#include <tobasaweb/controller_base.h>
#include <tobasaweb/router.h>
#include "../database_service_factory_app.h"

namespace tbs {

namespace web { class Session; }

namespace app {

class CoreController
   : public web::ControllerBase
{
public :
   CoreController( const CoreController & ) = delete;
   CoreController( CoreController && ) = delete;

   explicit CoreController(
      app::DbServicePtr dbService
   );

   ~CoreController() {}

   //! Handle GET request to /admin
    http::ResultPtr onAdmin(const web::RouteArgument& arg);

   //! Handle GET request to http server status page /spage/{statusCode:int}
   http::ResultPtr onSpage(const web::RouteArgument& arg);

   //! Handle GET request to / or index page
   http::ResultPtr onIndex(const web::RouteArgument& arg);

   //! Handle GET request to /dashboard
   http::ResultPtr onDashboard(const web::RouteArgument& arg);

   //! Handle GET request to /server_status
   http::ResultPtr onServerStatus(const web::RouteArgument& arg);

   //! Handle GET/POST request to /login
   http::ResultPtr onLogin(const web::RouteArgument& arg);

   //! Handle GET request to /logout
   http::ResultPtr onLogout(const web::RouteArgument& arg);

   //! Handle GET/POST request to /register
   http::ResultPtr onRegister(const web::RouteArgument& arg);

   //! Handle GET/POST request to /password
   http::ResultPtr onPassword(const web::RouteArgument& arg);

   //! Handle GET request to /resource/{resource_type}/{fileName}
   http::ResultPtr onAppResources(const web::RouteArgument& arg);

   //! Handle GET request to /keep_alive
   http::ResultPtr onKeepAlive(const web::RouteArgument& arg);

   //! Handle GET request to /user_profile/{profileId}
   http::ResultPtr onUserProfile(const web::RouteArgument& arg);   

protected:
   void bindHandler();

   //! Authenticate http context
   bool authenticate(
         const http::HttpContext& context,
         const std::string& userName,
         const std::string& password);

   http::ResultPtr redirectOnLoggedIn(std::shared_ptr<web::Session> session);

   app::DbServicePtr _dbService {nullptr};

};

} // namespace app
} // namespace tbs
