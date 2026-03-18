#pragma once

#include <tobasaweb/controller_base.h>
#include <tobasaweb/router.h>
#include "../database_service_factory_app.h"

namespace tbs {
namespace web { class WebappAgent;}
namespace app {

class ApiCoreController
   : public web::ControllerBase
{
public:
   ApiCoreController(const ApiCoreController &) = delete;
   ApiCoreController(ApiCoreController &&) = delete;

   explicit ApiCoreController(app::DbServicePtr dbService, std::shared_ptr<web::WebappAgent> webapp);
   virtual ~ApiCoreController();

   //! Handle GET request to /api/version
   http::ResultPtr onVersion(const web::RouteArgument& arg);

   //! Handle GET request to /api/server_status
   http::ResultPtr onApiServerStatus(const web::RouteArgument& arg);

   //! Handle POST request to /api/authenticate
   http::ResultPtr onAuthenticate(const web::RouteArgument& arg);

   //! Handle POST request to /api/refresh_auth_token
   http::ResultPtr onRefreshAuthToken(const web::RouteArgument& arg);

   //! Handle GET request to /api/decrypt/?data=clear_text
   //! Handle GET request to /api/encrypt/?data=encrypted_text
   http::ResultPtr onDecryptEncrypt(const web::RouteArgument& arg);

   //! Handle GET request to /api/read_log/{size}/{source}
   http::ResultPtr onReadLog(const web::RouteArgument& arg);

   //! Handle GET request to /api/running_configuration
   http::ResultPtr onGetAppConfig(const web::RouteArgument& arg);   

protected:

   void bindHandler();

   app::DbServicePtr _dbService {nullptr};
   std::shared_ptr<web::WebappAgent> _webappAgent = nullptr;
};


} // namespace app
} // namespace tbs