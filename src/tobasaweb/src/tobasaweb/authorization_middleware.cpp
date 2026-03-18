#include <tobasa/config.h>
#include <tobasahttp/server/common.h>
#include <tobasahttp/request.h>
#include "tobasaweb/settings_webapp.h"
#include "tobasaweb/util.h"
#include "tobasaweb/middleware.h"
#include "tobasaweb/credential_info.h"
#include "tobasaweb/json_result.h"
#include "tobasaweb/alert.h"
#include "tobasaweb/db_repo_auth_base.h"
#include "tobasaweb/authorization_middleware.h"

namespace tbs {
namespace web {

AuthorizationMiddleware::AuthorizationMiddleware()
{
   _name = "AuthorizationMiddleware";
   _option.cookieAuthOption.loginPath  = "/login";
   _option.cookieAuthOption.logoutPath = "/logout";
}

void AuthorizationMiddleware::option(AuthorizationMiddlewareOption option)
{
   _option = std::move(option);
}

void AuthorizationMiddleware::applyHttpResult(
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

http::RequestStatus AuthorizationMiddleware::invoke(const http::HttpContext& context)
{
   using namespace tbs::http;

   auto authContext = context->request()->authContext();

   if (authContext->authenticationIgnored)
      return next(context); // call next handler

   if (_option.ignoreHandler  && (_option.ignoreHandler)(context) )
   {
      authContext->authorizationIgnored = true;
      return next(context); // call next handler
   }

   auto response    = context->response();
   auto reqHeaders  = context->request()->headers();
   auto reqPath     = context->request()->path();

   if (authContext->disableCheck)
   {
      Logger::logT("[webapp] [conn:{}] {} {}", context->connId(), "AuthorizationMiddleware skipping (disable check):", reqPath );
      return next(context); // call next handler
   }

   if (authContext->effectiveScheme == AuthScheme::NONE)
   {
      Logger::logT("[webapp] [conn:{}] {} {}", context->connId(), "AuthorizationMiddleware skipping (auth scheme none):", reqPath );
      return next(context); // call next handler
   }

   if (authContext->effectiveScheme != AuthScheme::NONE)
   {
      const auto& authResult = std::any_cast<AuthResult&>(context->userData());

      if (!authResult.credentialsProvided)
      {
         StatusCode scode = StatusCode::UNAUTHORIZED;

         // User should provide credentials.
         if (util::startsWith(reqPath, "/api")  || authContext->effectiveScheme == AuthScheme::BEARER)
            response->addHeader("WWW-Authenticate", R"(Bearer realm="Credentials Required", charset="utf-8")");
         else
         {
            if (authContext->effectiveScheme == AuthScheme::COOKIE)
            {
               if (_option.cookieAuthOption.redirectToLoginPage)
                  scode = StatusCode::FOUND;
            }
            else
               response->addHeader("WWW-Authenticate", R"(Basic realm="Username/Password required", charset="utf-8")");
         }

         applyHttpResult(context, scode, "Unauthorized: no credential provided");
         return RequestStatus::handled; // Mark the request as processed, and stop here
      }

      std::string errorMessage;
      bool errorOccured  = false;
      bool hasPermission = [&]()
      {
         try
         {
            web::AuthDbRepoPtr authDbRepo = std::static_pointer_cast<web::AuthDbRepoBase>(dbServiceFactory()->getDbService("AuthDbRepo"));
            if (!authDbRepo)
               throw AppException("Failed to get AuthDbRepo");

            if ( ! authDbRepo->databaseConnected() )
               throw AppException("No connection to database");

            if (authResult.identity.pUser == nullptr)
               throw AppException("Invalid user pointer");

            Logger::logD("[webapp] [conn:{}] AuthorizationMiddleware: validating {} access", context->connId() , authResult.identity.pUser->userName);   

            auto userId         = authResult.identity.pUser->id;
            auto selectedSiteId = authResult.identity.selectedSiteId;

            if (authDbRepo->isSuperUser(userId))
               return true;

            if (authDbRepo->isSiteAdministrator(userId, selectedSiteId))
               return true;

            // check against request path
            if (authDbRepo->canAccessByRequestPathFullCheck(userId, selectedSiteId, std::string(reqPath), "all"))
            {
               Logger::logT("[webapp] [conn:{}] AuthorizationMiddleware: access granted by request path: {}", context->connId() , authResult.identity.pUser->userName);   
               return true;
            }
            else 
               Logger::logT("[webapp] [conn:{}] AuthorizationMiddleware: access denied by path: {}", context->connId() , authResult.identity.pUser->userName);   

            // check against path template
            auto pathArg = context->request()->pathArgument();
            if ( pathArg && authDbRepo->canAccessByRequestPathFullCheck(userId, selectedSiteId, pathArg->templatePath(), "all") ) 
            {
               Logger::logT("[webapp] [conn:{}] AuthorizationMiddleware: access granted by request path template: {}", context->connId() , authResult.identity.pUser->userName);   
               return true;
            }
            else
               Logger::logT("[webapp] [conn:{}] AuthorizationMiddleware: access denied by path template : {}", context->connId() , authResult.identity.pUser->userName);   
         }
         catch(const SqlException& ex )
         {
            errorMessage = "Database related error occured";
            errorOccured = true;
            Logger::logE("[webapp] [conn:{}] AuthorizationMiddleware: {}", context->connId(), ex.what());
         }
         catch(const AppException& ex )
         {
            Logger::logE("[webapp] [conn:{}] AuthorizationMiddleware: {}", context->connId(), ex.what());
            errorMessage = ex.what();
            errorOccured = true;
         }
         catch (const std::exception& ex)
         {
            Logger::logE("[webapp] [conn:{}] AuthorizationMiddleware: {}", context->connId(), ex.what());
            errorMessage = "Internal server error";
            errorOccured = true;
         }

         return false;
      }();

      if (!hasPermission)
      {
         StatusCode scode;
         if (errorOccured)
         {
            if (authContext->effectiveScheme == AuthScheme::COOKIE)
               scode = StatusCode::FOUND;
            else
               scode = StatusCode::INTERNAL_SERVER_ERROR;

            applyHttpResult(context, scode, errorMessage);
            return RequestStatus::handled; // Mark the request as processed, and stop here
         }
         else
         {
            // TODO_JEFRI: Better to use 403 Forbidden → When authentication is valid, but access is denied.
            // 401 Unauthorized → When authentication is missing or invalid.
            
            scode = StatusCode::UNAUTHORIZED;
            if (authContext->effectiveScheme == AuthScheme::COOKIE)
            {
               if (_option.cookieAuthOption.redirectToLoginPage)
                  scode = StatusCode::FOUND;
            }

            applyHttpResult(context, scode, "You don't have permission");
            return RequestStatus::handled; // Mark the request as processed, and stop here
         }
      }
   }

   // call next handler
   return next(context);
}

} // namespace http
} // namespace tbs