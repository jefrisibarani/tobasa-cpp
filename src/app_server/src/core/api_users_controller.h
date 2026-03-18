#pragma once

#include <tobasaweb/controller_base.h>
#include <tobasaweb/router.h>
#include "../database_service_factory_app.h"

namespace tbs {
namespace app {

class ApiUsersController
   : public web::ControllerBase
{
public:
   ApiUsersController( const ApiUsersController & ) = delete;
   ApiUsersController( ApiUsersController && ) = delete;

   explicit ApiUsersController(
      app::DbServicePtr dbService
   );

   ~ApiUsersController() {}

   //! Handle POST request to /api/users/authenticate
   http::ResultPtr onAuthenticate(const web::RouteArgument& arg);

   //! Handle POST request to /api/users/register
   http::ResultPtr onRegister(const web::RouteArgument& arg);

   //! Handle POST request to /api/users/register_with_image
   http::ResultPtr onRegisterWithImage(const web::RouteArgument& arg);

   //! Handle GET request to /api/users
   http::ResultPtr onGetAll(const web::RouteArgument& arg);

   //! Handle GET request to /api/users/{user_id:int}
   http::ResultPtr onGetById(const web::RouteArgument& arg);

   //! Handle GET request to /api/users/exists/{user_name}
   http::ResultPtr onUserExists(const web::RouteArgument& arg);

   //! Handle PUT request to /api/users/update_profile
   http::ResultPtr onUpdateProfile(const web::RouteArgument& arg);

   //! Handle POST request to /api/users/update_profile_with_image
   http::ResultPtr onUpdateProfileWithImage(const web::RouteArgument& arg);

   //! Handle DELETE request to /api/users/delete
   http::ResultPtr onDelete(const web::RouteArgument& arg);

   //! Handle GET request to /api/users/{user_id:int}/roles
   http::ResultPtr onRoles(const web::RouteArgument& arg);

   //! Handle GET request to /api/users/{user_id:int}/profile_image
   http::ResultPtr onProfileImage(const web::RouteArgument& arg);

   //! Handle POST request to /api/users/change_password
   http::ResultPtr onChangePassword(const web::RouteArgument& arg);

   //! Handle POST request to /api/users/check_password
   http::ResultPtr onCheckPassword(const web::RouteArgument& arg);

   //! Handle POST request to /api/users/reset_password
   http::ResultPtr onResetPassword(const web::RouteArgument& arg);

   //! Handle POST request to /api/users/forgot_password
   http::ResultPtr onForgotPassword(const web::RouteArgument& arg);

protected:
   virtual void bindHandler();

   app::DbServicePtr _dbService {nullptr};
};


} // namespace app
} // namespace tbs