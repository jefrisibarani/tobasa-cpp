#include <fstream>
#include <tobasa/config.h>
#include <tobasa/path.h>
#include <tobasa/uuid.h>
#include <tobasahttp/mimetypes.h>
#include <tobasahttp/server/status_page.h>
#include <tobasaweb/settings_webapp.h>
#include <tobasaweb/json_result.h>
#include <tobasaweb/alert.h>
#include <tobasaweb/session.h>
#include <tobasaweb/credential_info.h>
#include "core_controller.h"
#include "../api_result.h"
#include "../app_resource.h"
#include "../page.h"
#include "../app_common.h"
#include "../app_util.h"

namespace tbs {
namespace app {

using namespace http;
using namespace web;

std::string extensionToMimeType(const std::string& ext)
{
   return mimetypes::fromExtension(ext);
}

   CoreController::CoreController(
         app::DbServicePtr dbService
      )
      : web::ControllerBase()
      , _dbService {dbService}
   {}


void CoreController::bindHandler()
{
   using namespace std::placeholders;
   auto self(this);

   //! Handle GET request to homepage.
   router()->httpGet("/",
      std::bind(&CoreController::onIndex, self, _1) );

   //! Handle GET request to admin page
   router()->httpGet("/admin",
      std::bind(&CoreController::onAdmin, self, _1),        AuthScheme::BASIC);

   //! Handle GET request to http server status page
   router()->httpGet("/spage",
      std::bind(&CoreController::onSpage, self, _1) );

   //! Handle GET request to http server status page
   router()->httpGet("/spage/{statusCode:int}",
      std::bind(&CoreController::onSpage, self, _1) );

   //! Handle GET request to /server_status page
   router()->httpGet("/server_status",
      std::bind(&CoreController::onServerStatus, self, _1), AuthScheme::COOKIE);

   //! Handle GET request to /dashboard page
   router()->httpGet("/dashboard",
      std::bind(&CoreController::onDashboard, self, _1),    AuthScheme::COOKIE);

   //! Handle GET request to /login page
   router()->httpGet("/login",
      std::bind(&CoreController::onLogin, self, _1) );

   //! Handle POST request to /login page
   router()->httpPost("/login",
      std::bind(&CoreController::onLogin, self, _1) );

   //! Handle GET request to /logout?redirect=false
   router()->httpGet("/logout",
      std::bind(&CoreController::onLogout, self, _1) );

   //! Handle GET request to /register page
   router()->httpGet("/register",
      std::bind(&CoreController::onRegister, self, _1) );

   //! Handle POST request to /register page
   router()->httpPost("/register",
      std::bind(&CoreController::onRegister, self, _1) );

   //! Handle GET request to /password page
   router()->httpGet("/password",
      std::bind(&CoreController::onPassword, self, _1) );

   //! Handle POST request to /password page
   router()->httpPost("/password",
      std::bind(&CoreController::onPassword, self, _1) );

   //! Handle GET request to /user_profile
   router()->httpGet("/user_profile",
      std::bind(&CoreController::onUserProfile, self, _1), AuthScheme::COOKIE);

   //! Handle GET request to /user_profile/{profileId}
   router()->httpGet("/user_profile/{profileId}",
      std::bind(&CoreController::onUserProfile, self, _1), AuthScheme::COOKIE);

   //! Handle GET request to /resource/{resource_type}/{fileName}
   router()->httpGet("/resource/{resource_type}/{fileName}",
      std::bind(&CoreController::onAppResources, self, _1), AuthScheme::COOKIE);

   //! Handle GET request to /keep_alive
   router()->httpGet("/keep_alive",
      std::bind(&CoreController::onKeepAlive, self, _1),    AuthScheme::COOKIE);

   //! Server as Router default handler
   router()->defaultHandler(
      std::bind(&CoreController::onIndex, self, _1) );
}

//! Handle GET request to / or index page
http::ResultPtr CoreController::onIndex(const web::RouteArgument& arg)
{
   auto docRoot = app::docRoot();
   auto templateDir = app::templateDir();

   if (!web::conf::Webapp::useInMemoryResources)
   {
      if ( !path::exists(docRoot) || !path::exists(templateDir) )
      {
         // The router might have a ResultContentBuilder,
         // which could use files on disk (wwwroot or templates).
         // Therefore, we are setting the Result to not use that ResultContentBuilder.
         tbs::Logger::logE("wwwroot or views folder does not exist");
         auto result = statusResultHtml(StatusCode::INTERNAL_SERVER_ERROR);
         result->ignoreContentBuilder();
         return result;
      }
   }

   auto httpCtx = arg.httpContext();
   std::string requestPath = httpCtx->request()->path();
   if (requestPath.empty()) {
      return statusResultHtml(StatusCode::BAD_REQUEST);
   }

   // Request path must be absolute and not contain "..".
   if (requestPath.empty() || requestPath[0] != '/' || requestPath.find("..") != std::string::npos) {
      return statusResultHtml(StatusCode::FORBIDDEN);
   }

   std::string_view reqPathv {requestPath};
   if (util::startsWith(reqPathv, "/api") ) {
      return makeResult<JsonResult>(StatusCode::NOT_FOUND);
   }

   // If path ends in slash (i.e. is a directory) then add "index.html".
   if (requestPath[requestPath.size() - 1] == '/') {
      requestPath += "index.html";
   }

   if (requestPath == "/index.html")
   {
      // for index.html, load from template
      auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
      auto page = std::make_shared<web::Page>(httpCtx);
      return page->show("index.tpl");
   }
   else
   {
      if (!web::conf::Webapp::useInMemoryResources)
      {
         std::string fullPath = docRoot + requestPath;
         if (!path::exists(fullPath))
            return statusResultHtml(StatusCode::NOT_FOUND);

         return http::fileResult(fullPath);
      }

#ifdef TOBASA_BUILD_IN_MEMORY_RESOURCES
      if (web::conf::Webapp::useInMemoryResources)
      {
         std::string path = "wwwroot" + requestPath;
         auto content = app::Resource::get(path, "wwwroot");

         if (content.size() > 0)
            return rawBytesResult(content, extensionToMimeType( path::getExtension(path) ) );
         else
         {
            // Fallback. find in disk's wwwroot
            std::string fullPath = docRoot + requestPath;
            if (path::exists(fullPath))
               return http::fileResult(fullPath);
         }
      }
#endif

      return statusResultHtml(StatusCode::NOT_FOUND);
   }
}

//! Handle GET request to /dashboard
http::ResultPtr CoreController::onDashboard(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto session = Session::get(httpCtx->sessionId());
   auto alert   = Alert::create(httpCtx->sessionId());

   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   auto page = std::make_shared<web::Page>(httpCtx);

   page->data("pageLang",         "en");
   page->data("pageTitle",        "Dashboard - Tobasa Web Service");
   page->data("pageBodyClass",    "bg-primary");
   page->data("pageId",           "dashboard");
   page->data("pageContentTitle", "Dashboard");
   page->data("sessExpNotice",    60); // seconds
   page->data("sessExpTime",      authResult.expirationTime);
   page->data("pageBreadcrumb",   "Home / Dashboard");
   page->data()["identity"]["userName"]  = authResult.identity.pUser->userName;

   return page->show("dashboard.tpl");
}

//! Handle GET request to /admin page
http::ResultPtr CoreController::onAdmin(const web::RouteArgument& arg)
{
   return statusResultHtml(StatusCode::NOT_FOUND, "Admin page is not available now");
}

//! Handle GET request to /spage/{statusCode:int}  page
http::ResultPtr CoreController::onSpage(const web::RouteArgument& arg)
{
   auto arg0 = arg.get(0);
   auto code = arg.get("statusCode");
   if (code)
   {
      auto scode = code.value();
      if ( !util::isNumber(scode ) )
         return statusResultHtml(StatusCode::BAD_REQUEST, "Invalid value for parameter http status code");

      auto statusCode = static_cast<http::StatusCode>( std::stoi(scode) );
      return statusResultHtml(statusCode);
   }
   else
      return statusResultHtml(StatusCode::BAD_REQUEST, "Invalid parameter for http status code");
}

//! Handle GET request to /resource/{resource_type}/{fileName}
http::ResultPtr CoreController::onAppResources(const web::RouteArgument& arg)
{
   auto rt = arg.get("resource_type");
   auto fn = arg.get("fileName");
   if (!(rt && fn ))
      return statusResultHtml(StatusCode::BAD_REQUEST);

   std::string resType = rt.value();
   std::string file = fn.value();
   std::string filePath;

   if (resType == "data")
      filePath = app::dataDir() + path::SEPARATOR + file;
   else if (resType == "image")
      filePath = app::imageDir() + path::SEPARATOR + file;
   else if (resType == "upload")
      filePath = app::uploadDir() + path::SEPARATOR + file;
   else if (resType == "report")
      filePath = app::reportDir() + path::SEPARATOR + file;
   else if (resType == "images_user" || resType == "images_carousel" || resType == "images_doctor")
   {
      std::string defaultImage("_default_image.jpg");
      if (resType == "images_user")
         defaultImage = "_default_person.jpg";
      if (resType == "images_carousel")
         defaultImage = "_default_slide.jpg";
      if (resType  == "images_doctor")
         defaultImage = "_default_doctor.jpg";

      bool useApiResultOnError = false;
      return app::getImageResource(file,resType,defaultImage,useApiResultOnError);
   }
   else {
      filePath = app::dataDir() + path::SEPARATOR + resType + path::SEPARATOR + file;
   }

   if (!path::exists(filePath))
      return statusResultHtml(StatusCode::NOT_FOUND, "file not found");

   return http::fileResult(filePath);
}

//! Handle GET request to /keep_alive
http::ResultPtr CoreController::onKeepAlive(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto session = Session::get(httpCtx->sessionId());

   // Session expiration time check is done in Session Middleware
   auto& authResult = std::any_cast<AuthResult&>( httpCtx->userData() );
   long long timeLeft = authResult.expirationTime - DateTime::now().toUnixTimeSeconds(); // seconds
   if (timeLeft < 56)
   {
      std::string refreshToken = _dbService->createAuthDbRepo()->generateRefreshToken(authResult.identity.pUser);
      // refresh authentication session and browser cookies
      app::setupSessionAndCookie(httpCtx,refreshToken);
      timeLeft = authResult.expirationTime - DateTime::now().toUnixTimeSeconds(); // seconds
   }

   // Get the remaining seconds of the session
   return web::object(timeLeft);
}

//! Handle GET request to /user_profile/{profileId}
http::ResultPtr CoreController::onUserProfile(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto session = Session::get(httpCtx->sessionId());
   auto alert   = Alert::create(httpCtx->sessionId());
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );

   auto page = std::make_shared<web::Page>(httpCtx);

   page->data("pageLang",         "en");
   page->data("pageTitle",        "User Profile - Tobasa Web Service");
   page->data("pageBodyClass",    "bg-primary");
   page->data("pageId",           "user_profile");
   page->data("pageContentTitle", "User Profile");
   page->data("sessExpNotice",    60); // seconds
   page->data("sessExpTime",      authResult.expirationTime);
   page->data("pageBreadcrumb",   "Home / User profile");
   page->data("userData",  web::entity::UserDto(authResult.identity.pUser));

   page->data()["identity"]["userName"]  = authResult.identity.pUser->userName;

   auto contactName = authResult.identity.pUser->firstName + " " + authResult.identity.pUser->lastName;
   page->data()["identity"]["contactName"]  = contactName;

   return page->show("user_profile.tpl");
}

//! Handle GET request to /server_status
http::ResultPtr CoreController::onServerStatus(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto session = Session::get(httpCtx->sessionId());
   auto alert   = Alert::create(httpCtx->sessionId());

   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   auto page = std::make_shared<web::Page>(httpCtx);

   page->data("pageTitle",        "Server Status - Tobasa Web Service");
   page->data("pageBodyClass",    "");
   page->data("sessExpNotice",    60); // seconds
   page->data("sessExpTime",      authResult.expirationTime);
   page->data()["identity"]["userName"]  = authResult.identity.pUser->userName;

   return page->show("server_status.tpl");
}

//! Handle GET request to /password page
http::ResultPtr CoreController::onPassword(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto session = Session::get(httpCtx->sessionId());
   auto alert   = Alert::create(httpCtx->sessionId());

   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   if (authResult.loggedIn())
      return redirect("/");

   auto showChangePasswordPage =
   [httpCtx,alert](const std::string& message="", bool error=false)
   {
      if (!message.empty())
      {
         if (error) alert->error(message);
         else alert->info(message);
      }
      auto page = std::make_shared<web::Page>(httpCtx);
      page->title("Change password - Tobasa Web Service");
      return page->show("password.tpl");
   };


   if (httpCtx->request()->method() ==  "GET") {
      return showChangePasswordPage();
   }
   else if (httpCtx->request()->method() == "POST")
   {
      auto formBody = httpCtx->request()->formBody();
      if ( formBody->empty() )
         return statusResultHtml(StatusCode::BAD_REQUEST);

      std::string username = formBody->value("username");
      if (username.empty() || username.length() < 4 )
         return showChangePasswordPage("User name minimal length is 4 characters", true);

      auto& authResult = std::any_cast<AuthResult&>( httpCtx->userData() );
      auto user = _dbService->createAuthDbRepo()->getUserByName(username);
      if (user)
      {
         alert->success(tbsfmt::format("{}, please check your email: {}", user->firstName, user->email), Alert::LOC_TOAST, false);
         return redirect("/login");
      }
      else
      {
         session->setData("logged_in", false);
         return showChangePasswordPage("invalid user name - user not found", true);
      }
   }
   else
   {
      return statusResultHtml(StatusCode::BAD_REQUEST);
   }
}


//! Authenticate http context
bool CoreController::authenticate(
      const http::HttpContext& context,
      const std::string& userName,
      const std::string& password)
{
   std::string errorMessage;
   std::string exceptionMessage;

   // Get a reference to additional data.
   auto& authResult = std::any_cast<AuthResult&>( context->userData() );

   try
   {
      web::dto::LoginUser userDto;
      userDto.userName = userName;
      userDto.password = password;

      auto authDbRepo = _dbService->createAuthDbRepo();

      auto pUser = authDbRepo->authenticate(userDto);
      if (pUser != nullptr)
      {
         authResult.identity = web::Identity{
            pUser->id, // user id
            1,         // site id
            "id-ID",   // language id
            pUser      // user smart pointer
         };

         authResult.credentialsValid = true;

         web::AuthLogPtr authlog = std::make_shared<web::AuthLog>();
         authlog->logonTime = DateTime::now();
         authlog->usrId     = pUser->id;
         authlog->usrName   = pUser->userName;
         authlog->textNote  = "Core Module;/login";
         authlog->srcIp     = context->remoteEndpoint().address().to_string();
         authlog->srcHost   = "";
         authlog->srcMac    = "";
         authlog->authType  = "COOKIE";
         authlog->siteId    = 1;

         authDbRepo->logUserLogon(authlog);

         return true;
      }
   }
   catch(const AppException& ex )
   {
      exceptionMessage = ex.what();
      Logger::logE("[webapp] [conn:{}] authenticate, {}", context->connId(), ex.what());
   }
   catch(const std::exception& ex )
   {
      exceptionMessage = "Internal server error";;
      Logger::logE("[webapp] [conn:{}] authenticate, {}", context->connId(), ex.what());
   }

   if (!errorMessage.empty() || !exceptionMessage.empty())
   {
      authResult.credentialsValid = false;
      authResult.errorMessage = errorMessage;

      if (!exceptionMessage.empty())
         authResult.errorMessage = exceptionMessage;
   }

   return false;
}


http::ResultPtr CoreController::redirectOnLoggedIn(std::shared_ptr<Session> session)
{
   // get previous/current request target
   auto reqTarget = session->getData("request_target");
   if (reqTarget.empty())
      return redirect("/");
   else
   {
      auto target = reqTarget.get<std::string>();
      if (target == "/login")
         return redirect("/");
      else
         return redirect( target );
   }
}


//! Handle GET request to /logout?redirect=false
http::ResultPtr CoreController::onLogout(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto query = httpCtx->request()->query();

   app::logoutAndClearCookie(*this, httpCtx);
   if ( !query->empty() && query->hasField("redirect") )
   {
      auto redirect = query->value("redirect");
      if (redirect.length()>0)
      {
         if (redirect == std::string("false"))
            return web::okResult();
      }
   }

   auto page = std::make_shared<web::Page>(httpCtx);
   page->title("Logged out - Tobasa");
   page->data("pageBodyClass", "");

   return redirect("/");
}


//! Handle GET/POST request to /login page
http::ResultPtr CoreController::onLogin(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();

   std::string reqMethod = httpCtx->request()->method();
   auto session = Session::get(httpCtx->sessionId());
   auto alert   = Alert::create(httpCtx->sessionId());

   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   if (authResult.loggedIn()) {
      return redirectOnLoggedIn(session);
   }

   if ( reqMethod == "GET")
   {
      auto page = std::make_shared<web::Page>(httpCtx);
      page->title("Login - Tobasa Web Service");
      return page->show("login.tpl");
   }

   if (reqMethod == "POST")
   {
      auto formBody = httpCtx->request()->formBody();
      if ( formBody->empty() )
         return statusResultHtml(StatusCode::BAD_REQUEST);

      std::string userName = formBody->value("loginName");
      std::string password = formBody->value("password");
      std::string langId   = formBody->value("langId");
      std::string siteId   = formBody->value("siteId");

      if (!(langId=="id-ID" || langId=="en-US"))
         langId = "en-US";

      auto& authResult = std::any_cast<AuthResult&>( httpCtx->userData() );
      if (authenticate(httpCtx, userName, password))
      {
         auto authDbRepo = _dbService->createAuthDbRepo();

         std::string refreshToken = authDbRepo->generateRefreshToken(authResult.identity.pUser);
         app::setupSessionAndCookie(httpCtx, refreshToken);

         // Access token, we don't want access token stored in cookie
         std::string accessToken  = authDbRepo->generateAccessToken(authResult.identity.pUser);
         httpCtx->response()->addHeader("X-TBS-Api-Access-Token", accessToken);

         alert->success(tbsfmt::format("Welcome, {}", userName));
         return redirectOnLoggedIn(session);
      }
      else
      {
         session->setData("logged_in", false);
         alert->error(authResult.errorMessage);

         auto page = std::make_shared<web::Page>(httpCtx);
         page->title("Login - Tobasa Web Service");
         page->data("pageId", "id_login");
         page->data("isLoggedIn", false);
         return page->show("login.tpl");
      }
   }

   return statusResultHtml(StatusCode::BAD_REQUEST);
}


//! Handle GET request to /register page
http::ResultPtr CoreController::onRegister(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto session = Session::get(httpCtx->sessionId());
   auto alert   = Alert::create(httpCtx->sessionId());

   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );
   if (authResult.loggedIn())
      return redirect("/");

   auto showRegisterPage =
   [httpCtx,alert](const std::string& message="", bool error=false, const entity::UserDto& dto={})
   {
      if (!message.empty())
      {
         if (error) alert->error(message);
         else alert->info(message);
      }
      auto page = std::make_shared<web::Page>(httpCtx);
      page->title("Register - Tobasa Web Service");
      page->data("userData", dto);

      return page->show("register.tpl");
   };

   try
   {
      if (httpCtx->request()->method() == "GET") {
         return showRegisterPage();
      }
      else if (httpCtx->request()->method() == "POST")
      {
         auto formBody = httpCtx->request()->formBody();
         if ( formBody->empty() )
            return statusResultHtml(StatusCode::BAD_REQUEST);

         web::entity::User newUser;
         newUser.userName       = formBody->value("userName");
         newUser.firstName      = formBody->value("firstName");
         newUser.lastName       = formBody->value("lastName").empty() ? newUser.firstName : formBody->value("lastName");
         newUser.email          = formBody->value("email");
         newUser.phone          = formBody->value("phone");
         newUser.gender         = formBody->value("gender");
         newUser.address        = formBody->value("address");
         newUser.nik            = formBody->value("nik");
         newUser.selectedSiteId = 1;
         newUser.selectedLangId = "id-ID";
         newUser.enabled        = true;

         //newUser.uniqueCode   = formBody->value("uniqueCode");
         newUser.uniqueCode     = uuid::generate();

         DateTime dtBirthDate;
         if (! dtBirthDate.parse(formBody->value("birthDate"), "%Y-%m-%d") )
         {
            // create shared pointer by moving newUSer
            auto p = std::make_shared<entity::User>(std::move(newUser));
            auto userDto = entity::UserDto( p );
            return showRegisterPage("invalid birth date format", true, userDto);
         }

         newUser.birthDate = dtBirthDate;

         std::string password;
         std::string password0 = formBody->value("password0");
         std::string password1 = formBody->value("password1");
         if (password0 == password1)
            password = password0;
         else
         {
            // create shared pointer by moving newUSer
            auto p = std::make_shared<entity::User>(std::move(newUser));
            auto userDto = entity::UserDto( p );
            return showRegisterPage("invalid password", true, userDto);
         }


         //std::string devCodeOnly = formBody->value("testCode");
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

         try
         {
            bool checkAllRequirements = true;
            auto authDbRepo = _dbService->createAuthDbRepo();
            auto pUser = authDbRepo->enrollNewUSer(newUser, password, checkAllRequirements);
            if (pUser == nullptr)
            {
               session->setData("logged_in", false);
               return showRegisterPage("Registration failed");
            }

            // Possible returned values are:
            // VALID_UNIQUE_CODE, INVALID_UNIQUE_CODE, ERROR_UNIQUE_CODE_NOT_FOUND, ERROR_PARAMETER

            // TODO_JEFRI:
            // Validate user medical record number and unique code
            // std::string fullName  = newUser.firstName + " " + newUser.lastName; // contactName
            //std::string uniqueCodeResult = userService()->validateUniqueCode(newUserDto.uniqueCode, fullName, newUserDto.birthDate, newUserDto.nik);
            std::string uniqueCodeResult = "VALID_UNIQUE_CODE";

            // TODO_JEFRI
            //auto securitySalt = Config::getOption<std::string>("securitySalt");
            //userService()->sendActivationTokenEmail(pUser->id, pUser->userName, securitySalt);

            alert->success(tbsfmt::format("Congratulation, {}", pUser->userName));

            return redirect("/login");
         }
         catch(const ValidationException& ex)
         {
            // Note : create shared pointer by moving newUSer
            auto p = std::make_shared<entity::User>(std::move(newUser));
            auto userDto = entity::UserDto( p );
            return showRegisterPage(ex.what(), true, userDto);
         }
      }
      else
         return statusResultHtml(StatusCode::BAD_REQUEST);
   }
   catch(const std::exception& ex )
   {
      Logger::logE("[webapp] [conn:{}] onRegister, {}", httpCtx->connId(), ex.what());
      return statusResultHtml(StatusCode::INTERNAL_SERVER_ERROR);
   }
}

} // namespace app
} // namespace tbs