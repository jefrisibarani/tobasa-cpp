#include <fstream>
#include <tobasa/logger.h>
#include <tobasa/config.h>
#include <tobasa/util.h>
#include <tobasa/util_string.h>
#include <tobasa/path.h>
#include <tobasa/json.h>
#include <tobasasql/exception.h>
#include <tobasaweb/dto/login_user.h>
#include <tobasaweb/entity/user.h>
#include <tobasahttp/mimetypes.h>
#include <tobasaweb/settings_webapp.h>
#include <tobasaweb/json_result.h>
#include "api_result.h"
#include "../app_common.h"
#include "../app_util.h"
#include "api_users_controller.h"

namespace tbs {
namespace app {

using namespace http;
using namespace web;

   ApiUsersController::ApiUsersController(app::DbServicePtr dbService)
      : web::ControllerBase()
      , _dbService {dbService} {}

void ApiUsersController::bindHandler()
{
   auto self(this);
   using namespace std::placeholders;

   //! Handle POST request to /api/users/authenticate
   router()->httpPost("/api/users/authenticate",
      std::bind(&ApiUsersController::onAuthenticate, self, _1) );

   //! Handle POST request to /api/users/register
   router()->httpPost("/api/users/register",
      std::bind(&ApiUsersController::onRegister, self, _1) );

   //! Handle POST request to /api/users/register_with_image
   router()->httpPost("/api/users/register_with_image",
      std::bind(&ApiUsersController::onRegisterWithImage, self, _1) );

   //! Handle GET request to /api/users
   router()->httpGet("/api/users",
      std::bind(&ApiUsersController::onGetAll, self, _1),         AuthScheme::BEARER);

   //! Handle GET request to /api/users/{user_id:int}
   router()->httpGet("/api/users/{user_id:int}",
      std::bind(&ApiUsersController::onGetById, self, _1),        AuthScheme::BEARER);

   //! Handle GET request to /api/users/exists/{user_name}
   router()->httpGet("/api/users/exists/{user_name}",
      std::bind(&ApiUsersController::onUserExists, self, _1),     AuthScheme::BEARER);

   //! Handle PUT request to /api/users/update_profile
   router()->httpPut("/api/users/update_profile",
      std::bind(&ApiUsersController::onUpdateProfile, self, _1),  AuthScheme::BEARER);

   //! Handle POST request to /api/users/update_profile_with_image
   router()->httpPost("/api/users/update_profile_with_image", 
      std::bind(&ApiUsersController::onUpdateProfileWithImage, self, _1),  AuthScheme::BEARER);

   //! Handle DELETE request to /api/users/delete
   router()->httpDelete("/api/users/delete",
      std::bind(&ApiUsersController::onDelete, self, _1),         AuthScheme::BEARER);

   //! Handle GET request to /api/users/{user_id:int}/roles
   router()->httpGet("/api/users/{user_id:int}/roles",
      std::bind(&ApiUsersController::onRoles, self, _1),          AuthScheme::BEARER);

   //! Handle GET request to /api/users/{user_id:int}/profile_image
   router()->httpGet("/api/users/{user_id:int}/profile_image", 
      std::bind(&ApiUsersController::onProfileImage, self, _1),   AuthScheme::BEARER);

   //! Handle POST request to /api/users/change_password
   router()->httpPost("/api/users/change_password",
      std::bind(&ApiUsersController::onChangePassword, self, _1), AuthScheme::BEARER);

   //! Handle POST request to /api/users/check_password
   router()->httpPost("/api/users/check_password",
      std::bind(&ApiUsersController::onCheckPassword, self, _1),  AuthScheme::BEARER);

   //! Handle POST request to /api/users/reset_password
   router()->httpPost("/api/users/reset_password",
      std::bind(&ApiUsersController::onResetPassword, self, _1),  AuthScheme::NONE);

   //! Handle POST request to /api/users/forgot_password
   router()->httpPost("/api/users/forgot_password",
      std::bind(&ApiUsersController::onForgotPassword, self, _1), AuthScheme::NONE);
}


//! Handle POST request to /api/users/authenticate
ResultPtr ApiUsersController::onAuthenticate(const RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   if (!util::startsWith(httpCtx->request()->contentType(), "application/json"))
      return web::badRequest("invalid content type");

   auto body       = httpCtx->request()->content();
   auto jsonDto    = Json::parse(body);
   auto loginDto   = jsonDto.get<dto::LoginUser>();

   auto authDbRepo = _dbService->createAuthDbRepo();

   auto pUser = authDbRepo->authenticate(loginDto);
   if (!pUser)
      return web::badParameter("Username atau password tidak sesuai");

   // Log User Logon activity
   web::AuthLogPtr authlog = std::make_shared<web::AuthLog>();
   authlog->logonTime = DateTime::now();
   authlog->usrId     = pUser->id;
   authlog->usrName   = pUser->userName;
   authlog->textNote  = "Core Module;/api/users/authenticate";
   authlog->srcIp     = httpCtx->remoteEndpoint().address().to_string();
   authlog->srcHost   = "";
   authlog->srcMac    = "";
   authlog->authType  = "BEARER";
   authlog->siteId    = 1;

   authDbRepo->logUserLogon(authlog);

   /*
   send HTTP status code 200 with JSON response
   
   {
      "code":200,
      "message":"OK",
      "result": 
      {
         "accessToken": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpX...",
         "refreshToken": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpX..."
      }
   }
   
   */
   // stateless JWT refresh tokens
   // see /api/refresh_auth_token
   
   // short-lived: 5 min
   auto accessToken  = authDbRepo->generateAccessToken(pUser);
   // long-lived
   auto refreshToken = authDbRepo->generateRefreshToken(pUser);
   
   Json result;
   result["accessToken"]  = accessToken;
   result["refreshToken"] = refreshToken;
   
   return web::object(result);
}


//! Handle GET request to /api/users
http::ResultPtr ApiUsersController::onGetAll(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto usersVector = _dbService->createUserAclDbRepo()->getUsers();
   Json result(usersVector);
   return web::object(result);
}


//! Handle GET request to /api/users/{user_id:int}
http::ResultPtr ApiUsersController::onGetById(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto parId = arg.get("user_id");
   if (!parId)
      return web::badParameter("Invalid value for parameter user_id ");

   if ( !util::isNumber(parId.value()) )
      return web::badParameter("Invalid value for parameter user_id");   
   
   long userId = std::stol(parId.value());
   auto user   = _dbService->createAuthDbRepo()->getUserById(userId);

   if (!user) {
      return web::notFound("User not found");
   }

   Json userJson = entity::UserDto(user);
   return web::object(userJson);
}


//! Handle GET request to /api/users/exists/{user_name}
http::ResultPtr ApiUsersController::onUserExists(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();

   auto userName = arg.get("user_name");
   if (!userName)
      return web::badParameter("Invalid parameter user_name");

   bool exists = _dbService->createAuthDbRepo()->exists(*userName);
   if (exists)
      return web::okResult("ACCOUNT_EXISTS");
   else
      return web::okResult("ACCOUNT_NOT_FOUND");
}


//! Handle PUT request to /api/users/update_profile
http::ResultPtr ApiUsersController::onUpdateProfile(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   if (!util::startsWith(httpCtx->request()->contentType(), "application/json"))
      return web::badRequest("invalid content type");

   auto jsonDto = Json::parse(httpCtx->request()->content());

   DateTime birthDate;
   if (!birthDate.parse(jsonDto["birthDate"], "%Y-%m-%d"))
      return web::badParameter("Format tanggal lahir tidak valid");

   web::entity::User updtUser;

   updtUser.id          = jsonDto["id"];
   updtUser.userName    = jsonDto["userName"];
   updtUser.firstName   = jsonDto["firstName"];
   updtUser.lastName    = jsonDto["lastName"];
   updtUser.email       = jsonDto["email"];
   updtUser.uniqueCode  = jsonDto["uniqueCode"];
   updtUser.birthDate   = birthDate;
   updtUser.phone       = jsonDto["phone"];
   updtUser.gender      = jsonDto["gender"];
   updtUser.address     = jsonDto["address"];
   updtUser.nik         = jsonDto["nik"];
   updtUser.image       = jsonDto["DO_NOT_UPDATE"];

   auto pUser = _dbService->createAuthDbRepo()->updateUserProfile(updtUser);
   if (pUser)
      return web::okResult("User updated");
   else
      return web::appError("Gagal mengupdate user");
}


//! Handle POST request to /api/users/update_profile_with_image
http::ResultPtr ApiUsersController::onUpdateProfileWithImage(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   if (! httpCtx->request()->hasMultipartBody() )
      return web::badRequest();

   auto multipart = httpCtx->request()->multipartBody();

   DateTime dtBirthDate;
   if (!dtBirthDate.parse(multipart->value("birthDate"), "%Y-%m-%d"))
      return web::badParameter("Invalid format for parameter birth date");

   std::string newUserImagePath;
   auto profileImagePart = multipart->find("_image");
   if ( !profileImagePart )
      return web::badParameter("Invalid value for parameter profile image");

   if ( profileImagePart && profileImagePart->isFile )
   {
      auto extension = path::fileExtension(profileImagePart->fileName);
      util::strLower(extension);
      newUserImagePath = multipart->value("userName") + "_" + util::getRandomString(16);
      newUserImagePath.append("." + extension );
   }

   std::string lastName = multipart->value("lastName");
   web::entity::User updtUser;
   updtUser.id          = std::stol(multipart->value("userId"));
   updtUser.userName    = multipart->value("userName");
   updtUser.firstName   = multipart->value("firstName");
   updtUser.lastName    = lastName.empty() ? updtUser.firstName : lastName;
   updtUser.email       = multipart->value("emailAddress");
   updtUser.image       = newUserImagePath;
   updtUser.uniqueCode  = multipart->value("uniqueCode");
   updtUser.birthDate   = dtBirthDate;
   updtUser.phone       = multipart->value("phone");
   updtUser.gender      = multipart->value("gender");
   updtUser.address     = multipart->value("address");
   updtUser.nik         = multipart->value("nik");

   std::string olgImage = multipart->value("_oldimage");

   auto pUser = _dbService->createAuthDbRepo()->updateUserProfile(updtUser);
   if (pUser)
   {
      if ( profileImagePart->isFile ) 
      {
         std::string destinationDir = app::dataDir() + path::SEPARATOR + "images_user";
         
         if (!path::exists(destinationDir))
            path::createDir(destinationDir);

         std::string errMessage;
         bool stored = app::copyFileToDisk(destinationDir, newUserImagePath, profileImagePart->location, errMessage );
         if (!stored)
            Logger::logE("[webapp] [conn:{}] onUpdateProfileWithImage, Failed to save uploaded profile image: {}", httpCtx->connId(), errMessage );
         else
         {
            std::string errorMessage;
            std::string oldImagePath = destinationDir + path::SEPARATOR + olgImage;
            if (!path::removeFile(oldImagePath, errorMessage))
               Logger::logE("[webapp] [conn:{}] onUpdateProfileWithImage, Could not delete old profile image: {}, {}", httpCtx->connId(), oldImagePath, errorMessage );
         }
      }

      return web::okResult("User updated");
   }
   else
      return web::appError("Gagal mengupdate user");
}


//! Handle DELETE request to /api/users/delete
http::ResultPtr ApiUsersController::onDelete(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   if (!util::startsWith(httpCtx->request()->contentType(), "application/json"))
      return web::badRequest("invalid content type");

   auto jsonDto = Json::parse(httpCtx->request()->content());
   
   if (!jsonDto.contains("user_id"))
      return web::badParameter("Invalid parameter value for user id");

   long uid = jsonDto["user_id"].get<long>();

   if ( _dbService->createAuthDbRepo()->remove(uid))
      return web::okResult("User deleted");
   else
      return web::appError("Gagal menghapus user");
}


//! Handle GET request to /api/users/{user_id:int}/roles
http::ResultPtr ApiUsersController::onRoles(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto parId = arg.get("user_id");
   if (parId)
      return web::badParameter("Invalid value for parameter user id");

   if ( !util::isNumber(parId.value()) )
      return web::badParameter("Invalid value for parameter user id");

   auto authDbRepo = _dbService->createAuthDbRepo();

   long userId = std::stol(parId.value());
   if (! authDbRepo->exists(userId))
      return web::notFound("User not found");

   auto userRoles = authDbRepo->getUserRoleDto(userId);
   return web::object(userRoles);
}


//! Handle GET request to /api/users/{user_id:int}/profile_image
http::ResultPtr ApiUsersController::onProfileImage(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();

   auto parId = arg.get("user_id");
   if (!parId)
      return web::badParameter("Invalid value for parameter user id");

   entity::UserPtr userPtr = nullptr; 
   auto authDbRepo = _dbService->createAuthDbRepo();

   // Try with user id which is an integer
   if ( util::isNumber(parId.value()) )
   {
      long userId = std::stol(parId.value());
      userPtr = authDbRepo->getUserById(userId);
   }
   else
   {
      std::string uuid = parId.value();
      userPtr = authDbRepo->getUserByUuid(uuid);
   } 

   if (!userPtr)
      return web::notFound("User not found");

   std::string userImg = userPtr->image;
   return app::getImageResource(userImg, "IMG_USER", "_default_person.jpg");
}


//! Handle POST request to /api/users/change_password
http::ResultPtr ApiUsersController::onChangePassword(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   if (!util::startsWith(httpCtx->request()->contentType(), "application/json"))
      return web::badRequest("invalid content type");

   auto jsonDto = Json::parse(httpCtx->request()->content());
   if (jsonDto.contains("id") && jsonDto.contains("userName") && jsonDto.contains("currentPassword") && jsonDto.contains("newPassword"))
   {
      auto userId          = jsonDto["id"];
      auto userName        = jsonDto["userName"];
      auto currentPassword = jsonDto["currentPassword"];
      auto newPassword     = jsonDto["newPassword"];
      
      bool success = _dbService->createAuthDbRepo()->changePassword(userName, currentPassword, newPassword);
      if(success)
         return web::okResult("Password successfully updated");
      else
         return web::failed("Update password failed");
   }
   else
      return web::okResult("Invalid json data for parameters");
}


//! Handle POST request to /api/users/check_password
http::ResultPtr ApiUsersController::onCheckPassword(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   if (!util::startsWith(httpCtx->request()->contentType(), "application/json"))
      return web::badRequest("invalid content type");

   auto jsonDto  = Json::parse(httpCtx->request()->content());
   
   if (jsonDto.contains("userName") && jsonDto.contains("password"))
   {
      auto userName = jsonDto["userName"];
      auto password = jsonDto["password"];

      bool success = _dbService->createAuthDbRepo()->checkPassword(userName, password);
      if (success)
      {
         /*
         send HTTP status code 200 with JSON response
            {
               "code": 200,
               "message": "User/Password valid",
               "result": {
                  "additionalInfo": "VALID_CREDENTIAL"
               }
            }
         */
         Json res;
         res["additionalInfo"] = "VALID_CREDENTIAL";
         return web::object(res, "User/Password valid");   
      }
      else
         return web::failed("Update password failed");
   }
   else
      return web::okResult("Invalid json data for parameters");
}


//! Handle POST request to /api/users/reset_password
http::ResultPtr ApiUsersController::onResetPassword(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   if (!util::startsWith(httpCtx->request()->contentType(), "application/json"))
      return web::badRequest("invalid content type");

   auto jsonDto = Json::parse(httpCtx->request()->content());
   if (jsonDto.contains("userName") && jsonDto.contains("newPassword") && jsonDto.contains("resetCode") )
   {
      auto userName    = jsonDto["userName"];
      auto newPassword = jsonDto["newPassword"];
      auto resetCode   = jsonDto["resetCode"];

      bool success = _dbService->createAuthDbRepo()->resetPassword(userName, newPassword, resetCode);
      if (success)
         return web::okResult("Password reset successfully");
      else
         return web::failed("Reset password failed");
   }
   else
      return web::badParameter("Invalid parameter user name/new password/reset code");
}


//! Handle POST request to /api/users/forgot_password
http::ResultPtr ApiUsersController::onForgotPassword(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   if (!util::startsWith(httpCtx->request()->contentType(), "application/json"))
      return web::badRequest("invalid content type");

   auto jsonDto = Json::parse(httpCtx->request()->content());
   if (jsonDto.contains("userName") && jsonDto.contains("emailAddress"))
   {
      auto userName     = jsonDto["userName"];
      auto emailAddress = jsonDto["emailAddress"];

      auto expiredTime = DateTime();
      expiredTime.timePoint() +=  std::chrono::minutes{60};  //60min;
      
      bool success = _dbService->createAuthDbRepo()->forgotPassword(userName, emailAddress, expiredTime);
      if (!success)
         return web::failed("Password reset request failed");

      long long expiredUnixTimeStampMilis = expiredTime.toUnixTimeMiliSeconds();
      auto timeLeftMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::minutes{60});
      Json res;
      res["requestExpiredTime"] = expiredUnixTimeStampMilis;
      res["requestDuration"]    = timeLeftMs.count();

      return web::object(res, "Please check your email for password reset code");
   }
   else
      return web::badParameter("Invalid parameter user name/email address");
}



   // -------------------------------------------------------
   // Registration stuff
   // -------------------------------------------------------

namespace {
   ResultPtr doRegisterNewUser(
         web::AuthDbRepoPtr authDbRepo
         , entity::UserCreateDto& newUserDto
         , http::HttpContext httpCtx
      )
   {
      bool hasMultipart = httpCtx->request()->hasMultipartBody();
   
      // When registering a user via the Mobile App, the birthDate, uniqueCode, phone, address, and nik must not be empty.
      // The userName, firstName, lastName, and address will be validated by the AuthDbRepo.
      
      if (newUserDto.uniqueCode.empty())
         return web::badParameter("Invalid value for parameter unique code");
   
      if (newUserDto.phone.empty())
         return web::badParameter("Invalid value for parameter phone number");
   
      if (newUserDto.address.empty())
         return web::badParameter("Invalid value for parameter address");
   
      if (newUserDto.nik.empty())
         return web::badParameter("Invalid value for parameter nik");
   
      if (newUserDto.birthDate.empty())
         return web::badParameter("Invalid value for parameter birth date");
      
      DateTime dtBirthDate;
      if (! dtBirthDate.parse(newUserDto.birthDate, "%Y-%m-%d") )
         return web::badParameter("Invalid format for parameter birth date");

      // remove non printable char
      util::removeTrailingWhiteSpace(newUserDto.nik);

      web::entity::User newUser;
      newUser.userName       = newUserDto.userName;
      newUser.firstName      = newUserDto.firstName;
      newUser.lastName       = newUserDto.lastName.empty() ? newUserDto.firstName : newUserDto.lastName;
      newUser.email          = newUserDto.email;
      newUser.image          = "";
      newUser.uniqueCode     = newUserDto.uniqueCode;
      newUser.birthDate      = dtBirthDate;
      newUser.phone          = newUserDto.phone; 
      newUser.gender         = newUserDto.gender;
      newUser.address        = newUserDto.address;
      newUser.nik            = newUserDto.nik;
      newUser.selectedSiteId = newUserDto.selectedSiteId;
      newUser.selectedLangId = newUserDto.selectedLangId;
      newUser.enabled        = true;
      newUser.allowLogin     = false;  // we'll check again later....
   
      bool checkAllRequirements = true;
      MultipartBody::PartPtr profileImagePart = nullptr;
      std::string newUserImagePath;
   
      if ( hasMultipart )
      {
         // request came from /api/users/register_with_image
         checkAllRequirements = false;
   
         auto multipart = httpCtx->request()->multipartBody();
         // Process User Profile Image
   
         profileImagePart = multipart->find("profileImage");
         if ( profileImagePart == nullptr)
            return web::badParameter("Invalid value for parameter profile image");
   
         if ( profileImagePart && profileImagePart->isFile )
         {
            auto extension = path::fileExtension(profileImagePart->fileName);
            util::strLower(extension);
            newUserImagePath = newUserDto.userName + "_" + util::getRandomString(16);
            newUserImagePath.append("." + extension );
         }
   
         newUser.image = newUserImagePath;
      }
   
      // -------------------------------------------------------
      // Check if connection came from Tobasa Android App
      // Note: Another deeper check maybe?
      bool isCreatingChatUser = false;
      std::string clientAppId = httpCtx->request()->headers().value("X-Client-App-Id");
      std::string userAgent   = httpCtx->request()->headers().value("User-Agent");
      if ( app::isValidAppClientId(clientAppId) || util::contains(userAgent, "TBSMOBILEAPP_TOBASA") ) 
      {
         newUser.allowLogin   = true;
         newUser.uniqueCode   = util::generateUniqueId();
         isCreatingChatUser   = true;
      }
      // -------------------------------------------------------   
   
      entity::UserPtr pUser = authDbRepo->enrollNewUSer(newUser, newUserDto.password, checkAllRequirements);
      if (pUser == nullptr)
      {
         return web::failed("Failed to register user");
      }
   
      // Process user profile image
      if ( profileImagePart && profileImagePart->isFile ) 
      {
         std::string destinationDir = app::dataDir() + path::SEPARATOR + "images_user";
         if (!path::exists(destinationDir))
            path::createDir(destinationDir);
         
         std::string errMessage;
         bool stored = app::copyFileToDisk(destinationDir , newUserImagePath, profileImagePart->location, errMessage );
         if (!stored)
            Logger::logE("[webapp] [conn:{}] onRegisterWithImage, Failed to save profile image: {}", httpCtx->connId(), errMessage);
      }
   
      // Possible returned values are:
      // VALID_UNIQUE_CODE, INVALID_UNIQUE_CODE, ERROR_UNIQUE_CODE_NOT_FOUND, ERROR_PARAMETER
   
      // TODO_JEFRI:
      // Validate user medical record number and unique code
      // std::string fullName  = newUser.firstName + " " + newUser.lastName; // contactName
      //std::string uniqueCodeResult = authDbRepo->validateUniqueCode(newUserDto.uniqueCode, fullName, newUserDto.birthDate, newUserDto.nik);
      std::string uniqueCodeResult = "VALID_UNIQUE_CODE";
   
      // TODO_JEFRI
      //auto securitySalt = Config::getOption<std::string>("securitySalt");
      //authDbRepo->sendActivationTokenEmail(pUser->id, pUser->userName, securitySalt);

      return web::object(uniqueCodeResult);
   }
   
} // anonymous namespace


//! Handle POST request to /api/users/register_with_image
ResultPtr ApiUsersController::onRegisterWithImage(const RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   if ( ! httpCtx->request()->hasMultipartBody() )
      return web::badRequest();

   auto multipart = httpCtx->request()->multipartBody();

   entity::UserCreateDto newUserDto;
   newUserDto.userName        = multipart->value("userName");
   newUserDto.firstName       = multipart->value("firstName");
   newUserDto.lastName        = multipart->value("lastName");
   newUserDto.email           = multipart->value("emailAddress");
   newUserDto.password        = multipart->value("password");
   newUserDto.uniqueCode      = multipart->value("uniqueCode");
   newUserDto.birthDate       = multipart->value("birthDate");
   newUserDto.phone           = multipart->value("phone");
   newUserDto.gender          = multipart->value("gender");
   newUserDto.address         = multipart->value("address");
   newUserDto.nik             = multipart->value("nik");
   newUserDto.selectedSiteId  = std::stoi(multipart->value("selectedSiteId"));
   newUserDto.selectedLangId  = multipart->value("selectedLangId");

   //newUserDto.contactName   = multipart->value("contactName");

   return doRegisterNewUser(
      _dbService->createAuthDbRepo()
      , newUserDto
      , httpCtx
   );
}


//! Handle POST request to /api/users/register
ResultPtr ApiUsersController::onRegister(const RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   if (!util::startsWith(httpCtx->request()->contentType(), "application/json"))
      return web::badRequest("invalid content type");

   auto body       = httpCtx->request()->content();
   auto jsonDto    = Json::parse(body);
   auto newUserDto = jsonDto.get<entity::UserCreateDto>();
   
   return doRegisterNewUser(
    _dbService->createAuthDbRepo()
    , newUserDto
    , httpCtx
   );

}

} // namespace app
} // namespace tbs
