#include <tobasa/config.h>
#include <tobasa/uuid.h>
#include <tobasahttp/server/common.h>
#include <tobasahttp/request.h>
#include "tobasaweb/settings_webapp.h"
#include "tobasaweb/middleware.h"
#include "tobasaweb/credential_info.h"
#include "tobasaweb/json_result.h"
#include "tobasaweb/db_repo_auth_base.h"
#include "tobasaweb/util.h"
#include "tobasaweb/session_middleware.h"

namespace tbs {
namespace web {

namespace {

void updateSessionEndpoint(SessionPtr session, const http::HttpContext& context)
{
   // save client ip address
   std::ostringstream ostr;
   ostr << context->remoteEndpoint() ;
   session->setData("remote_endpoint", ostr.str() );
}

conf::RouteSession readSessionConfigurationRule(const std::vector<conf::RouteSession>& rules, const std::string& requestPath)
{
   for (auto route: rules)
   {
      if (route.check == "starts_with" )
      {
         if (util::startsWith(requestPath, route.path))
            return route;
      }
      else if (route.check == "ends_with" )
      {
         if (util::endsWith(requestPath, route.path))
            return route;
      }
      else // match
      {
         if (requestPath == route.path)
            return route;
      }
   }

   return {};
}

/** Read route without session rule from configuration
 * @param requestPath http request path
 */
bool sessionDisabledInConfigurationFile(const std::string& requestPath)
{
   auto appOption = Config::getOption<web::conf::Webapp>("webapp");
   auto rule = readSessionConfigurationRule(appOption.webService.noSessionList, requestPath);
   if (rule.path.empty() )
      return false;
   
   return true;
}

} // namespace

SessionMiddleware::SessionMiddleware()
{
   _name = "SessionMiddleware";
   _option.cookieAuthOption.loginPath  = "/login";
   _option.cookieAuthOption.logoutPath = "/logout";
}

void SessionMiddleware::option(SessionMiddlewareOption option)
{
   _option.cookieAuthOption = std::move(option.cookieAuthOption);
   _option.ignoreHandler    = std::move(option.ignoreHandler);
   _option.cookieRules      = std::move(option.cookieRules);
   _option.loginPathBuilder = std::move(option.loginPathBuilder);
}

void SessionMiddleware::applyHttpResult(
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
      if (util::startsWith(context->request()->path(), "/api"))
      {
         http::JsonResult result(statusCode, message);
         result.toResponse(context->response());
      }
      else
      {
         http::StatusResult result(statusCode, message);
         result.toResponse(context->response());
      }
   }
}

/// Skip session processing for AuthScheme::NONE,  request path '/' and login path
bool SessionMiddleware::skipSessionProcessing(const http::HttpContext& context)
{
   using namespace tbs::http;

   auto reqPath     = context->request()->path();
   auto authContext = context->request()->authContext();

   std::string loginPath = _option.cookieAuthOption.loginPath;
   if (_option.loginPathBuilder != nullptr) {
      loginPath = _option.loginPathBuilder(context);
   }

   if ( (authContext->effectiveScheme == AuthScheme::NONE) 
        && !(reqPath =="/" || reqPath == "/login" || reqPath == loginPath ) )
      return true;

   return false;
}


void SessionMiddleware::setupNewSession(const http::HttpContext& context)
{
   using namespace tbs::http;

   auto response         = context->response();
   std::string sessionId = uuid::generate();
   std::string cookieValue;
   
   if (_option.cookieRules.size() > 0) 
   {
      for (auto& cookie: _option.cookieRules) 
      {
         auto reqPath = context->request()->path();
         if ( util::startsWith(reqPath, cookie.path))
         {
            // TODO_JEFRI:
            cookieValue = cookie.generate();
            break;
         }
      }
   }
   else {
      cookieValue = Session::createDefaultCookie(sessionId);
   }
   
   auto session = Session::create(sessionId);
   context->request()->sessionId(sessionId);

   if (!cookieValue.empty()) {
      response->addHeader("Set-Cookie", cookieValue);
   }
   
   response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate");
   response->addHeader("Pragma",        "no-cache");
   response->addHeader("Expires",       "-1");

   session->setData("logged_in", false);

   std::string loginPath = _option.cookieAuthOption.loginPath;
   if (_option.loginPathBuilder != nullptr) {
      loginPath = _option.loginPathBuilder(context);
   }

   auto reqTarget = context->request()->target();
   if ( ! (reqTarget =="/keep_alive" || reqTarget == _option.cookieAuthOption.logoutPath || reqTarget == loginPath ) )
   {
      // save request target instead of request path into session cookie data
      // request target: /mypage/viewer?uid=894845
      // request path: /maypage/viewer
      // we will use this later, as a redirect target after a successfull login
      session->setData("request_target", reqTarget);
   }

   // save client ip address
   updateSessionEndpoint(session, context);
}


http::RequestStatus SessionMiddleware::invoke(const http::HttpContext& context)
{
   using namespace tbs::http;

   if (_option.ignoreHandler  && (_option.ignoreHandler)(context) )
      return next(context); // call next handler
   
   auto reqPath = context->request()->path();

   if (sessionDisabledInConfigurationFile(reqPath)) {
      return next(context); // call next handler
   }

   Logger::logT("[webapp] [conn:{}] {} {}", context->connId(), "Invoking session handler:", reqPath);

   // get cookie from request header
   auto cookie = context->request()->cookie();
   if (!cookie->empty())
   {
      std::string sessionId = cookie->value(Session::COOKIE_NAME);

      // we got cookie in request header, but with invalid values
      if ( sessionId.empty() || sessionId.length() != Session::COOKIE_ID_LENGTH)
      {
         if (skipSessionProcessing(context))
         {
            Logger::logT("[webapp] [conn:{}] {} {}", context->connId(), "SessionMiddleware skipping:", reqPath );
            return next(context); // call next handler
         }
         // create new session for this connection
         setupNewSession(context);
         return next(context); // call next handler
      }
      else
      {
         // valid cookies value, existing session
         if (sessionId.length() == Session::COOKIE_ID_LENGTH)
         {
            // set http context request session id
            context->request()->sessionId(sessionId);

            if (skipSessionProcessing(context))
            {
               Logger::logT("[webapp] [conn:{}] {} {}", context->connId(), "SessionMiddleware skipping: ", reqPath );
               return next(context); // call next handler
            }

            // get session data from session storage
            // session data may have been deleted by previous request to /logout
            auto session = Session::get(sessionId);
            if (!session->loaded())
            {
               Logger::logT("[webapp] [conn:{}] SessionMiddleware, {}", context->connId(), "Could not load session storage file");
               return next(context); // call next handler
            }

            // update sessions' remote_endpoint
            updateSessionEndpoint(session, context);

            // validate session storage, make sure session has not expired
            bool loggedIn               = false;
            bool stillValid             = false;
            long long expiresUtcSeconds = 0;
            tbs::Json expireTime        = session->getData("expires");

            if (!expireTime.empty())
            {
               expiresUtcSeconds = expireTime.get<long long>();
               if (expiresUtcSeconds > 0)
               {
                  std::chrono::system_clock::time_point tp{std::chrono::milliseconds{expiresUtcSeconds*1000}};
                  DateTime dtExpire(tp);
                  DateTime dtNow;
                  stillValid = dtExpire.timePoint() > dtNow.timePoint();
                  if (!stillValid)
                  {
                     Logger::logE("[webapp] [conn:{}] SessionMiddleware, {}", context->connId(), "Session storage already expired");
                     Session::destroy(sessionId);
                  }
               }
            }

            // check if this session is logged in
            auto logged = session->getData("logged_in");
            if (!logged.empty())
            {
               if (logged.get<bool>() == true )
                  loggedIn = true;
            }

            if ( !context->userData().has_value() )
            {
               // create user data if not exists. 
               context->userData( std::make_any<web::AuthResult>() );
            }

            if (loggedIn && stillValid)
            {
               // Read identity info from stored session data, then put into http context authResult
               auto& authResult = std::any_cast<AuthResult&>( context->userData() );
               auto userId      = session->getData("user_id");
               auto userName    = session->getData("user_name");
               auto siteId      = session->getData("selected_site_id");
               auto langId      = session->getData("selected_lang_id");

               if (!userName.empty())
               {
                  try
                  {
                     web::AuthDbRepoPtr authDbRepo = std::static_pointer_cast<web::AuthDbRepoBase>(dbServiceFactory()->getDbService("AuthDbRepo"));
                     if (!authDbRepo)
                        throw AppException("Failed to get AuthDbRepo");

                     if ( ! authDbRepo->databaseConnected() )
                        throw AppException("No connection to database");

                     // if we have broken db connection, getUserByName() will throw SqlException
                     std::string userNameStr = userName.get<std::string>();
                     auto pUser = authDbRepo->getUserByName(userNameStr);
                     if (pUser != nullptr)
                     {
                        auto selectedSiteId = siteId.get<int>();
                        auto selectedLangId = langId.get<std::string>();

                        authResult.identity = web::Identity {
                           pUser->id,      // user id
                           selectedSiteId, // site id
                           selectedLangId, // language id
                           pUser           // user smart pointer
                        };

                        authResult.expirationTime      = expiresUtcSeconds;
                        authResult.credentialsValid    = true;
                        authResult.credentialsProvided = true;
                     }
                     else
                     {
                        std::string errorMessage = "Could not retrieve user identity from database";
                        authResult.errorMessage = errorMessage;
                        Logger::logE("[webapp] [conn:{}] SessionMiddleware, {}", context->connId(), errorMessage);
                     }
                  }
                  catch(const SqlException& ex )
                  {
                     std::string errorMessage = "User identity could not be retrieved from the database due to a database error";
                     authResult.errorMessage = errorMessage;
                     Logger::logE("[webapp] [conn:{}] SessionMiddleware: active session found, but failed to get user identity from database because of database error: {}", context->connId(), ex.what());
                  }
                  catch(std::exception& e)
                  {
                     authResult.errorMessage = "User identity could not be retrieved from the database due to an internal server error";
                     Logger::logE("[webapp] [conn:{}] SessionMiddleware: An active session was found, but the retrieval of user identity from the database failed due to an internal server error: {}", context->connId(), e.what());
                     http::StatusResult result(http::StatusCode::INTERNAL_SERVER_ERROR);
                     result.toResponse(context->response());
                     return http::RequestStatus::handled;
                  }
               }

               session->setData("request_target", reqPath);
               // call next handler
               return next(context);
            }
            else
            {

            }
         }
      }
   }
   else
   {
      if (skipSessionProcessing(context))
      {
         Logger::logT("[webapp] [conn:{}] {} {}", context->connId(), "SessionMiddleware skipping: ", reqPath );
         return next(context); // call next handler
      }

      setupNewSession(context);
   }

   // call next handler
   return next(context);
}


} // namespace http
} // namespace tbs