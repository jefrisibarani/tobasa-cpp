#include <tobasa/config.h>
#include <tobasahttp/request.h>
#include <tobasahttp/status_codes.h>
#include "tobasaweb/jwt.h"
#include "tobasaweb/credential_info.h"
#include "tobasaweb/dto/login_user.h"
#include "tobasaweb/json_result.h"
#include "tobasaweb/settings_webapp.h"
#include "tobasaweb/alert.h"
#include "tobasaweb/util.h"
#include "tobasaweb/db_repo_auth_base.h"
#include "tobasaweb/authentication_middleware.h"

namespace tbs {
namespace web {

AuthenticationMiddleware::AuthenticationMiddleware()
{
   _name = "AuthenticationMiddleware";
   _option.cookieAuthOption.loginPath  = "/login";
   _option.cookieAuthOption.logoutPath = "/logout";
}

void AuthenticationMiddleware::option(AuthenticationMiddlewareOption option)
{
   _option = std::move(option); 
}

void AuthenticationMiddleware::applyHttpResult(
   const http::HttpContext& context,
   http::StatusCode statusCode,
   const std::string& message)
{
   if (_resultBuilder)
   {
      auto result = _resultBuilder(context, statusCode, message);
      result->toResponse(context->response());
   }
   else
   {
      auto effectiveScheme = context->request()->authContext()->effectiveScheme;
      if (effectiveScheme == http::AuthScheme::BEARER)
      {
         http::JsonResult result(statusCode, message);
         result.toResponse(context->response());
      }
      else if (effectiveScheme == http::AuthScheme::COOKIE)
      {
         if (statusCode == http::StatusCode::FOUND)
         {
            Logger::logT("[webapp] [conn:{}] {} ", context->connId(), "AuthenticationMiddleware: applyHttpResult: cookie code == FOUND" );

            std::string loginPath = _option.cookieAuthOption.loginPath;
            if (_option.loginPathBuilder != nullptr) {
               loginPath = _option.loginPathBuilder(context);
            }

            if (loginPath.empty())
               loginPath = "/login";

            auto alert = Alert::create(context->sessionId());
            alert->error(message);

            auto result = http::makeResult();
            result->contentType("");
            result->redirect(loginPath);
            result->statusCode(statusCode); 
            result->toResponse(context->response());
         }
         else
         {
            Logger::logT("[webapp] [conn:{}] {} ", context->connId(), "AuthenticationMiddleware: applyHttpResult: cookie code != FOUND" );
            http::StatusResult result(statusCode, message);
            result.toResponse(context->response());
         }
      }
      else
      {
         Logger::logT("[webapp] [conn:{}] {} ", context->connId(), "AuthenticationMiddleware: applyHttpResult: unknown scheme" );
         http::StatusResult result(statusCode, message);
         result.toResponse(context->response());
      }
   }
}

http::RequestStatus AuthenticationMiddleware::invoke(const http::HttpContext& context)
{
   using namespace tbs::http;

   auto authContext = context->request()->authContext();

   if (_option.ignoreHandler  && (_option.ignoreHandler)(context) )
   {
      authContext->authenticationIgnored = true;
      return next(context); // call next handler
   }

   auto response    = context->response();
   auto reqHeaders  = context->request()->headers();
   auto reqPath     = context->request()->path();

   if ( authContext->disableCheck ) 
   {
      Logger::logT("[webapp] [conn:{}] {} {}", context->connId(), "AuthenticationMiddleware skipping (disable check):", reqPath );
      return next(context); // call next handler
   }

   auto& authResult = std::any_cast<AuthResult&>( context->userData() );
   if ( authContext->effectiveScheme == AuthScheme::BASIC)
   {
      Logger::logD("[webapp] [conn:{}] {}", context->connId(), "Handling Basic auth scheme from Authorization header");
      std::string errorMessage;

      AuthParseResult parseResult = parseBasicAuth(context->request()->authContext()->rawText);
      if (parseResult.valid)
      {
         authResult.credentialsProvided = true;
         verifyBasicAuth(context, parseResult.username, parseResult.password);

         if ( !authResult.credentialsValid && !authResult.errorMessage.empty() )
         {
            std::string errorMsg = "Invalid Basic Authentication input : "  + authResult.errorMessage;
            response->addHeader("WWW-Authenticate", R"(Basic realm="Valid Username/Password should be provided", charset="utf-8")");
            applyHttpResult(context, StatusCode::UNAUTHORIZED, errorMsg);
            return RequestStatus::handled; // Mark the request as processed, and stop here
         }
      }
      else
      {
         // Parsing error occured
         std::string errorMsg = tbsfmt::format("Error when parsing Basic Authentication : {}", parseResult.errorMessage);
         Logger::logE("[webapp] [conn:{}] {}", context->connId(), errorMsg);
         response->addHeader("WWW-Authenticate", R"(Basic realm="Valid Username/Password should be provided", charset="utf-8")");
         applyHttpResult(context, StatusCode::UNAUTHORIZED, errorMsg);
         return RequestStatus::handled; // Mark the request as processed, and stop here;
      }
   }
   else if ( authContext->effectiveScheme == AuthScheme::BEARER)
   {
      std::string_view reqPathv {reqPath};
      if ( util::startsWith(reqPathv, "/api" ) || util::startsWith(reqPathv, "/chat_app_socket" ) )
      {
         Logger::logD("[webapp] [conn:{}] {}", context->connId(), "Handling Bearer auth scheme from Authentication header");

         if (!context->request()->authContext()->headerFound)
         {
            std::string errorMsg("Authentication header not found");
            Logger::logE("[webapp] [conn:{}] {}", context->connId(), errorMsg);
            applyHttpResult(context, StatusCode::UNAUTHORIZED, errorMsg);
            return RequestStatus::handled; // Mark the request as processed, and close connection
         }

         std::string jwtToken {context->request()->authContext()->rawText};
         if (jwtToken.length() > 0 && jwtToken != "null")
         {
            authResult.credentialsProvided = true;
            // verify the token and save the result into authResult
            this->verifyBearerAuth(context, jwtToken);

            if ( !authResult.credentialsValid && !authResult.errorMessage.empty() )
            {
               std::string errorMsg = "Auth JWT, "  + authResult.errorMessage;
               response->addHeader("WWW-Authenticate", R"(Bearer realm="Valid JWT should be provided", charset="utf-8")");
               applyHttpResult(context, StatusCode::UNAUTHORIZED, errorMsg);
               return RequestStatus::handled; // Mark the request as processed, and close connection
            }
         }
         else
         {
            std::string errorMsg("invalid JWT Bearer token format");
            Logger::logE("[webapp] [conn:{}] {}", context->connId(), errorMsg);
            applyHttpResult(context, StatusCode::UNAUTHORIZED, errorMsg);
            return RequestStatus::handled; // Mark the request as processed, and close connection
         }
      }
      else
         Logger::logW("[webapp] [conn:{}] {}", context->connId(), "Bearer Auth scheme found, request path invalid");
   }
   else if (authContext->effectiveScheme == AuthScheme::COOKIE)
   {
      Logger::logD("[webapp] [conn:{}] {}", context->connId(), "Handling Cookie auth scheme from Authorization header");

      if ( !authResult.credentialsValid )
      {
         if (authResult.errorMessage.empty())
            authResult.errorMessage = "Authentication required to access page " + reqPath;

         if ( _option.cookieAuthOption.redirectToLoginPage )
         {
            Logger::logE("[webapp] [conn:{}] AuthenticationMiddleware: redirect {} to login page", context->connId(), reqPath);
            
            // Session is not logged in
            // For the cookie auth scheme, instead of returning an unauthorized status page, 
            // we should redirect to the login page by setting the status code to StatusCode::FOUND
            applyHttpResult(context, StatusCode::FOUND, authResult.errorMessage);
         }
         else
            applyHttpResult(context, StatusCode::UNAUTHORIZED, authResult.errorMessage);
         
         return RequestStatus::handled; // Mark the request as processed, and close connection
      }
   }
   else
   {
      // AuthScheme::NONE or AuthScheme::CUSTOM
      if (_option.authHandler && !(_option.authHandler)(context,*this) )
      {
         applyHttpResult(context, StatusCode::UNAUTHORIZED, authResult.errorMessage);
         return RequestStatus::handled; // Mark the request as processed, and close connection
      }

      Logger::logT("[webapp] [conn:{}] {}", context->connId(), "No supported Authentication scheme found");
   }

   return next(context); // call next handler
}

//! Handler Basic Auth
void AuthenticationMiddleware::verifyBasicAuth(
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
      web::AuthDbRepoPtr authDbRepo = std::static_pointer_cast<web::AuthDbRepoBase>(dbServiceFactory()->getDbService("AuthDbRepo"));
      if (!authDbRepo)
         throw AppException("Failed to get AuthDbRepo");

      if ( ! authDbRepo->databaseConnected() )
         throw AppException("No connection to database");


      web::dto::LoginUser userDto;
      userDto.userName = userName;
      userDto.password = password;

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
         authlog->textNote  = "";
         authlog->srcIp     = context->remoteEndpoint().address().to_string();
         authlog->srcHost   = "";
         authlog->srcMac    = "";
         authlog->authType  = "BASIC";
         authlog->siteId    = 1;

         authDbRepo->logUserLogon(authlog);
      }
   }
   catch(const SqlException& ex )
   {
      exceptionMessage = "Database related error occured";
      Logger::logE("[webapp] [conn:{}] verifyBasicAuth, {}", context->connId(), ex.what());
   }   
   catch(const AppException& ex )
   {
      exceptionMessage = ex.what();
      Logger::logE("[webapp] [conn:{}] verifyBasicAuth, {}", context->connId(), ex.what());
   }
   catch(const std::exception& ex )
   {
      exceptionMessage = "Internal server error";;
      Logger::logE("[webapp] [conn:{}] verifyBasicAuth, {}", context->connId(), ex.what());
   }

   if (!errorMessage.empty() || !exceptionMessage.empty())
   {
      authResult.credentialsValid = false;
      authResult.errorMessage = errorMessage;

      if (!exceptionMessage.empty())
         authResult.errorMessage = exceptionMessage;
   }
}

//! Handle Bearer auth
void AuthenticationMiddleware::verifyBearerAuth(
         const http::HttpContext& context,
         const std::string& token,
         const std::string& username)
{
   // Get a reference to additional data.
   auto& authResult = std::any_cast<AuthResult&>( context->userData() );
   auto appOption   = Config::getOption<web::conf::Webapp>("webapp");
   auto wsOption    = appOption.webService;

   std::string accessTokenSecret  = wsOption.authJwtSecret;
   std::string jwtIssuer          = wsOption.authJwtIssuer;
   int jwtExpTimeSpanMinutes      = wsOption.authJwtExpireTimeSpanMinutes;

   std::string errorMessage;
   std::string exceptionMessage;

   try
   {
      web::AuthDbRepoPtr authDbRepo = std::static_pointer_cast<web::AuthDbRepoBase>(dbServiceFactory()->getDbService("AuthDbRepo"));
      if (!authDbRepo)
         throw AppException("Failed to get AuthDbRepo");

      if ( ! authDbRepo->databaseConnected() )
         throw AppException("No connection to database");

      auto decoded  = jwt::decode(token);
      auto verifier = jwt::verify()
                           .allow_algorithm(jwt::algorithm::hs256{ accessTokenSecret })
                           .with_issuer(jwtIssuer);

      std::error_code errCode;
      verifier.verify(decoded, errCode);
      if (!errCode)
      {
         int         userId;
         std::string uniqueName;
         int         selectedSiteId;
         std::string selectedLangId;
         // Extract values from jwt token
         for (auto& e : decoded.get_payload_json())
         {
            if (e.first == "user_id")
            {
               auto id = e.second.get<std::string>();
               userId = std::stoi(id);
            }

            if (e.first == "user_name")
               uniqueName = e.second.get<std::string>();

            if (e.first == "selected_site_id")
            {
               auto siteId = e.second.get<std::string>();
               selectedSiteId = std::stoi(siteId);
            }

            if (e.first == "selected_lang_id")
               selectedLangId = e.second.get<std::string>();
         }

         bool usernameVerified = false;
         // compare unique_name in JWT token to x-username header
         if (!username.empty() && !uniqueName.empty())
         {
            // user name provided, then we should compare with unique_name
            if (username == uniqueName) {
               usernameVerified = true;
            }
            else
            {
               errorMessage = "X-Username missmatch";
               Logger::logE("[webapp] [conn:{}] verifyBearerAuth, {}", context->connId(), errorMessage);
            }
         }
         else
         {
            // An empty username implies that there is no 'x-username' header, and the authentication process 
            // uses the 'Authorization' header, not 'x-token,' so we do not need to check for validity
            usernameVerified = true;
         }

         if (usernameVerified)
         {
            // if we have broken db connection, getUserByName() will throw SqlException

            // token verified, now compare with user database
            auto pUser = authDbRepo->getUserByName(uniqueName);
            if (pUser != nullptr)
            {
               authResult.identity = web::Identity {
                  pUser->id,      // user id
                  selectedSiteId, // site id
                  selectedLangId, // language id
                  pUser           // user smart pointer
               };

               authResult.credentialsValid = true;
            }
            else
            {
               errorMessage = "Invalid username";
               Logger::logE("[webapp] [conn:{}] verifyBearerAuth, {}", context->connId(), errorMessage);
            }
            
         }
      }
      else
      {
         errorMessage = errCode.message();
         Logger::logE("[webapp] [conn:{}] verifyBearerAuth, {}", context->connId(), errCode.message());
      }
   }
   catch (const Json::exception& ex)
   {
      exceptionMessage = ex.what();
      Logger::logE("[webapp] [conn:{}] verifyBearerAuth, {}", context->connId(), ex.what());
   }
   catch(const SqlException& ex )
   {
      exceptionMessage = "Database related error occured";
      Logger::logE("[webapp] [conn:{}] verifyBearerAuth, {}", context->connId(), ex.what());
   }   
   catch(const AppException& ex )
   {
      exceptionMessage = ex.what();
      Logger::logE("[webapp] [conn:{}] verifyBearerAuth, {}", context->connId(), ex.what());
   }
   catch(const std::exception& ex )
   {
      exceptionMessage = "Internal server error";
      Logger::logE("[webapp] [conn:{}] verifyBearerAuth, {}", context->connId(), ex.what());
   }

   if (!errorMessage.empty() || !exceptionMessage.empty())
   {
      authResult.credentialsValid = false;
      authResult.errorMessage = errorMessage;

      if (!exceptionMessage.empty())
         authResult.errorMessage = exceptionMessage;
   }
}


AuthenticationMiddleware::AuthParseResult AuthenticationMiddleware::parseBasicAuth(const std::string& authHeader)
{
   std::string errorMessage;
   std::string_view authHeaderV {authHeader};
   std::string userPasswordB64 {authHeaderV};

   try
   {
      using namespace jwt;
      auto userPassword = base::decode<alphabet::base64>(base::pad<alphabet::base64>(userPasswordB64));
      auto pos = userPassword.find_first_of(':');
      if (pos != std::string::npos)
      {
         AuthParseResult result;
         result.username = userPassword.substr(0,pos);
         result.password = userPassword.substr(pos+1);
         result.valid = true;
         return std::move(result);
      }
      else
         errorMessage = "Invalid basic auth username password format";
   }
   catch(const std::exception& e)
   {
      errorMessage = e.what();
   }

   AuthParseResult result;
   result.errorMessage = errorMessage;

   return std::move(result);
}


} // namespace http
} // namespace tbs