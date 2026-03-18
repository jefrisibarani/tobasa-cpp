#include <fstream>
#include <filesystem>
#include <tobasa/format.h>
#include <tobasa/config.h>
#include <tobasa/crypt.h>
#include <tobasa/path.h>
#include <tobasa/util.h>
#include <tobasa/json.h>
#include <tobasaweb/settings_webapp.h>
#include <tobasaweb/exception.h>
#include <tobasaweb/cookie.h>
#include <tobasaweb/jwt.h>
#include <tobasaweb/credential_info.h>
#include <tobasaweb/entity/user.h>
#include "tobasaweb/webapp_agent.h"
#include "api_core_controller.h"
#include "build_info.h"
#include "../api_result.h"
#include "../app_util.h"
#include "../app_common.h"

namespace tbs {
namespace app {

using namespace http;
using namespace web;

ApiCoreController::ApiCoreController(app::DbServicePtr dbService, std::shared_ptr<WebappAgent> webappAgent)
   : web::ControllerBase() 
   , _dbService {dbService}
   , _webappAgent { webappAgent } {}


ApiCoreController::~ApiCoreController() {}


void ApiCoreController::bindHandler()
{
   auto self(this);
   using namespace std::placeholders;

   //! Handle GET request to /api/version
   router()->httpGet("/api/version",
      std::bind(&ApiCoreController::onVersion, self, _1),           AuthScheme::NONE);

   //! Handle GET request to /api/server_status
   router()->httpGet("/api/server_status",
      std::bind(&ApiCoreController::onApiServerStatus, self, _1),   AuthScheme::BEARER);

   //! Handle POST request to /api/authenticate
   router()->httpPost("/api/authenticate",
      std::bind(&ApiCoreController::onAuthenticate, self, _1),      AuthScheme::NONE);

   //! Handle POST request to /api/refresh_auth_token
   router()->httpPost("/api/refresh_auth_token",
      std::bind(&ApiCoreController::onRefreshAuthToken, self, _1),  AuthScheme::NONE);

   //! Handle GET request to /api/encrypt?data=encrypted_text
   router()->httpGet("/api/encrypt",
      std::bind(&ApiCoreController::onDecryptEncrypt, self, _1),    AuthScheme::BEARER);

   //! Handle GET request to /api/decrypt?data=clear_text
   router()->httpGet("/api/decrypt",
      std::bind(&ApiCoreController::onDecryptEncrypt, self, _1),    AuthScheme::BEARER);

   //! Handle GET request to /api/read_log/{size:int}/{source}
   router()->httpGet("/api/read_log/{size:int}/{source}",
      std::bind(&ApiCoreController::onReadLog, self, _1),           AuthScheme::BEARER);

   //! Handle GET request to /api/running_configuration
   router()->httpGet("/api/running_configuration",
      std::bind(&ApiCoreController::onGetAppConfig, self, _1),      AuthScheme::BEARER);
}


//! Handle GET request to /api/version
http::ResultPtr ApiCoreController::onVersion(const web::RouteArgument& arg)
{
   // Main application sql connection object
   std::string versionStringMain =
      std::visit([&](auto& conn)
         { return conn->versionString(); }, _dbService->sqlConnPtrVariant() );

   Json resp;
   resp["server"]     = "Tobasa Web Service";
   resp["version"]    = tbs::appVersion();
   resp["build date"] = tbs::appBuildDate();
   resp["compiler"]   = tbs::compilerId() + " " + tbs::compilerVersion() + " " + tbs::compilerPlatform();
   resp["database"]   = versionStringMain;

   /*
   send HTTP status code 200 with JSON response
      {
         "code": 200,
         "message": "OK",
         "result": {
            "build date": "2025-10-29 02:31:00 UTC",
            "compiler": "MSVC 1916 x64",
            "database": "SQLite Version 3.39.2",
            "server": "Tobasa Web Service",
            "version": "0.8.0"
         }
      }
   */

   return web::object(resp);
}


//! Handle POST request to /api/refresh_auth_token
http::ResultPtr ApiCoreController::onRefreshAuthToken(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   if (!util::startsWith(httpCtx->request()->contentType(), "application/json"))
      return web::badRequest("invalid content type");

   try
   {
      std::string refreshToken;

      // get cookie from request header
      auto cookie = httpCtx->request()->cookie();

      if (!cookie->empty()) 
      {
         // find tbs_api_refresh_token in cookie
         refreshToken = cookie->value("tbs_api_refresh_token");
         if (refreshToken.empty())
            Logger::logE("[webapp] [conn:{}] onRefreshAuthToken, tbs_api_refresh_token not found in cookie", httpCtx->connId());
      }

      if (refreshToken.empty())
      {
         // find in request body
         auto jsonDto = Json::parse( httpCtx->request()->content(), nullptr, false );
         if (!jsonDto.is_discarded() && jsonDto.contains("refreshToken")) 
         {
            refreshToken = jsonDto["refreshToken"];
            if ( refreshToken.empty() )
               Logger::logE("[webapp] [conn:{}] onRefreshAuthToken, refreshToken not found in request body", httpCtx->connId());
         }
         else
            Logger::logE("[webapp] [conn:{}] onRefreshAuthToken, failed to parse request body", httpCtx->connId());
      }

      if (refreshToken.empty())
         return web::badRequest();

      auto appOption = Config::getOption<web::conf::Webapp>("webapp");
      auto wsOption  = appOption.webService;
      std::string refreshTokenSecret = wsOption.authJwtSecretRefresh;
      std::string jwtIssuer          = wsOption.authJwtIssuer;

      auto decoded  = jwt::decode(refreshToken);
      auto verifier = jwt::verify()
                           .allow_algorithm(jwt::algorithm::hs256{ refreshTokenSecret })
                           .with_issuer(jwtIssuer);

      std::string errorMessage;
      std::error_code errCode;
      verifier.verify(decoded, errCode);
      if (!errCode)
      {
         std::string userUuid;
         std::string uniqueName;

         for (auto& e : decoded.get_payload_json())
         {
            if (e.first == "user_uuid")
               userUuid = e.second.get<std::string>();

            if (e.first == "user_name")
               uniqueName = e.second.get<std::string>();
         }
         auto authDbRepo = _dbService->createAuthDbRepo();  

         // TODO_JEFRI: Refresh Token Rotation
         // Check if this refresh token is the latest one issued
         // auto latestStoredToken = authDbRepo->getLatestRefreshTokenFromDB(userUuid);
         // if (latestStoredToken !== refreshToken) {
         //    return return web::unauthorized("Stale refresh token");
         // }

         // if we have broken db connection, getUserByName() will throw SqlException
         // token verified, now compare with user database
         auto pUser = authDbRepo->getUserByName(uniqueName);
         if (pUser != nullptr)
         {
            //auto userRoleDto = authDbRepo->getUserRoleDto(pUser->id);
            // send HTTP status code 200 with JSON response
            auto accessToken  = authDbRepo->generateAccessToken(pUser);
            
            // we use stateless JWT refresh tokens, no need to generate the new one

            // TODO_JEFRI: Refresh Token Rotation
            //auto refreshToken = authDbRepo->generateRefreshToken(pUser);
            // Store the new refresh token (invalidate the old one)
            //authDbRepo->storeRefreshTokenInDB(pUser->id, refreshToken);

            Json result;
            result["accessToken"]  = accessToken;
            // TODO_JEFRI: Refresh Token Rotation

            result["projectId"]           = "XYZ";
            result["clientAppId"]         = "TBSRESTC_TOBASA";
            result["webserviceVersion"]   = "1.0.0";
            result["deviceToken"]         = "ABC";
            result["chatProtocolVersion"] = "1.0.0";
            result["userAgent"]           = "TBSMOBILEAPP_TOBASA/1.0";  

            return web::object(result);
         }
         else
         {
            errorMessage = "Invalid username";
            Logger::logE("[webapp] [conn:{}] onRefreshAuthToken, {}", httpCtx->connId(), errorMessage);
            return web::appError("Invalid user name");
         }
      }
      else
      {
         errorMessage = errCode.message();
         Logger::logE("[webapp] [conn:{}] onRefreshAuthToken, {}", httpCtx->connId(), errCode.message());
         return web::unauthorized();
      }
   }
   catch (const tbs::ValidationException& ex)
   {
      Logger::logE("[webapp] [conn:{}] {}", httpCtx->connId(), ex.what());
      return web::badParameter(ex.what());
   }
   catch (const SqlException& ex)
   {
      Logger::logE("[webapp] [conn:{}] {}", httpCtx->connId(), ex.what());
      return web::appError("SQL server error");
   }
   catch (const tbs::AppException& ex)
   {
      Logger::logE("[webapp] [conn:{}] {}", httpCtx->connId(), ex.what());
      return web::appError(ex.what());
   }
   catch (const Json::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] {}", httpCtx->connId(), ex.what());
      return web::badRequest("JSON data error: " + cleanJsonException(ex));
   }
   catch (const std::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] {}", httpCtx->connId(), ex.what());
      return web::appError("Unexpected error ooccured");
   }
}


//! Handle POST request to /api/authenticate
http::ResultPtr ApiCoreController::onAuthenticate(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   if (!util::startsWith(httpCtx->request()->contentType(), "application/json"))
      return web::badRequest("invalid content type");

   try
   {
      auto body     = httpCtx->request()->content();
      auto jsonDto  = Json::parse(body);
      auto loginDto = jsonDto.get<web::dto::LoginUser>();

      auto authDbRepo = _dbService->createAuthDbRepo();

      auto pUser    = authDbRepo->authenticate(loginDto);
      if (!pUser)
         return web::badParameter("Username atau password tidak sesuai");

      // Log User Logon activity
      web::AuthLogPtr authlog = std::make_shared<web::AuthLog>();
      authlog->logonTime = DateTime::now();
      authlog->usrId     = pUser->id;
      authlog->usrName   = pUser->userName;
      authlog->textNote  = "Core Module;/api/authenticate";
      authlog->srcIp     = httpCtx->remoteEndpoint().address().to_string();
      authlog->srcHost   = "";
      authlog->srcMac    = "";
      authlog->authType  = "BEARER";
      authlog->siteId    = loginDto.selectedSiteId;
      
      authDbRepo->logUserLogon(authlog);

      std::string uniqueCode = "INVALID_UNIQUE_CODE";
      if (!pUser->uniqueCode.empty() && pUser->uniqueCode.length() > 4)
      {
         // user supplied correct uniquecode upon registration or callcentre fix the uniquecode
         if (!loginDto.uniqueCode.empty() && loginDto.uniqueCode == pUser->uniqueCode)
            uniqueCode = pUser->uniqueCode;
         else if (loginDto.uniqueCode.empty() || loginDto.uniqueCode == "INVALID_UNIQUE_CODE")
            uniqueCode = pUser->uniqueCode;
      }

      auto userRoleDto  = authDbRepo->getUserRoleDto(pUser->id);
      auto accessToken  = authDbRepo->generateAccessToken(pUser);
      auto refreshToken = authDbRepo->generateRefreshToken(pUser);

      Json result;
      result["id"]             = pUser->id;
      result["userName"]       = pUser->userName;
      result["firstName"]      = pUser->firstName;
      result["lastName"]       = pUser->lastName;
      result["image"]          = pUser->image;
      result["selectedSiteId"] = loginDto.selectedSiteId;
      result["selectedLangId"] = loginDto.selectedLangId;
      result["uniqueCode"]     = uniqueCode;
      result["birthDate"]      = pUser->birthDate.isoDateString();
      result["phone"]          = pUser->phone;
      result["address"]        = pUser->address;
      result["gender"]         = pUser->gender;
      result["nik"]            = pUser->nik;
      result["email"]          = pUser->email;
      result["roles"]          = userRoleDto;
      result["accessToken"]    = accessToken;
      result["refreshToken"]   = refreshToken;

      return web::object(result);
   }
   catch (const tbs::ValidationException& ex)
   {
      Logger::logE("[webapp] [conn:{}] {}", httpCtx->connId(), ex.what());
      return web::badParameter(ex.what());
   }
   catch (const SqlException& ex)
   {
      Logger::logE("[webapp] [conn:{}] {}", httpCtx->connId(), ex.what());
      return web::appError("SQL server error");
   }
   catch (const tbs::AppException& ex)
   {
      Logger::logE("[webapp] [conn:{}] {}", httpCtx->connId(), ex.what());
      return web::appError(ex.what());
   }
   catch (const Json::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] {}", httpCtx->connId(), ex.what());
      return web::badRequest("JSON data error: " + cleanJsonException(ex));
   }
   catch (const std::exception& ex)
   {
      Logger::logE("[webapp] [conn:{}] {}", httpCtx->connId(), ex.what());
      return web::appError("Unexpected error ooccured");
   }
}


//! Handle GET request to /api/decrypt?data=clear_text
//! Handle GET request to /api/encrypt?data=encrypted_text
http::ResultPtr ApiCoreController::onDecryptEncrypt(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto appOption = Config::getOption<web::conf::Webapp>("webapp");
   auto securitySalt = Config::getOption<std::string>("securitySalt");
   auto query = httpCtx->request()->query();

   if ( !query->empty() && query->hasField("data") )
   {
      auto data = query->value("data");
      if (data.length()>0)
      {
         if (httpCtx->request()->path() == "/api/decrypt")
         {
            auto decrypted = data;
            auto clearText = crypt::passwordDecrypt(decrypted, securitySalt);
            return web::object(clearText);
         }
         else if (httpCtx->request()->path() == "/api/encrypt")
         {
            auto clearText = data;
            auto decrypted = crypt::passwordEncrypt(clearText, securitySalt);

            /*
            send HTTP status code 200 with JSON response
                  {
                     "code": 200,
                     "message": "OK",
                     "result": "5FC9F87D7C3566C43410A1335C45BAA5"
                  }
            */
            return web::object(decrypted);
         }
         else
            return web::notFound("Requested service not found");
      }
      else
         return web::badParameter("Invalid parameter data");
   }
   else
   {
      /*
      send HTTP status code 400 with JSON response
         {
            "code": 400,
            "message": "Query string not found"
         }
      */
      return web::badRequest("Query string not found");
   }
}


//! Handle GET request to /api/read_log/{size}/{source}
http::ResultPtr ApiCoreController::onReadLog(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto size    = arg.get("size");
   auto source  = arg.get("source");

   if (size && source)
   {
      std::string filterVal = source.value();

      if ( !util::isNumber(size.value()) )
         return web::badParameter("Invalid value for parameter size");

      if (! (    filterVal == "LIS"  || filterVal == "ALL" || filterVal == "SQL"
              || filterVal == "INFO" || filterVal == "ERROR" || filterVal == "DEBUG" || filterVal == "TRACE" || filterVal == "WARN" ) )
         return web::badParameter("Invalid value for parameter source");

      try
      {
         std::string logFilePath = Logger::logFilePath();
         auto totalLine = std::stoi(size.value());

         if (filterVal == "SQL")
            filterVal = "[sql]";
         else if (filterVal == "LIS")
            filterVal = "[lis_";
         else if (filterVal == "INFO")
            filterVal = "[info]";
         else if (filterVal == "ERROR")
            filterVal = "[error]";
         else if (filterVal == "DEBUG")
            filterVal = "[debug]";
         else if (filterVal == "TRACE")
            filterVal = "[trace]";
         else if (filterVal == "WARN")
            filterVal = "[warn]";
         else
            filterVal = "ALL";

         if (totalLine > 0)
         {
            // get only last totalLine
            Json resp = Json::array();

            if (! app::getFileContentFromEnd(logFilePath, totalLine, resp, filterVal) )
               return web::appError("Error creating ifstream");

            return web::object(resp);
         }
         else
         {
            auto fileSize = path::fileSize(logFilePath);
            if (fileSize > (40*104858)) // 40 MB
               return web::appError("Log file too big to open");

            // get all content
            std::ifstream ifs(logFilePath, std::ifstream::in);
            if (!ifs.good())
               return web::appError("Error creating ifstream");

            Json resp = Json::array();
            std::string line;
            while (std::getline(ifs, line))
            {
               if ( filterVal == "ALL" )
                  resp.emplace_back(line);
               else
               {
                  if ( line.find( filterVal ) != std::string::npos )
                     resp.emplace_back(line);
               }
            }

            ifs.close();
            return web::object(resp);
         }
      }
      catch (std::exception& ex)
      {
         Logger::logE("[webapp] [conn:{}] {}", httpCtx->connId(), ex.what());
         return web::appError("Error getting log file content");
      }
   }
   else
      return web::badParameter("Invalid parameter");
}


//! Handle GET request to /api/server_status
http::ResultPtr ApiCoreController::onApiServerStatus(const web::RouteArgument& arg)
{
   _webappAgent->updateStatus();
   web::WebappStatus appStatus = _webappAgent->status();

   std::string appcompilerInfo = tbs::compilerId() + " " + tbs::compilerVersion() + " " + tbs::compilerPlatform();
   std::string appNameVersion  = "Tobasa Web Service " + tbs::appVersion();
   std::string appBuildDate    = tbs::appBuildDate();

   // Main application sql connection object
   std::string dbInformation =
      std::visit([&](auto& conn)
         { return conn->versionString(); }, _dbService->sqlConnPtrVariant() );

   std::string dbName =
      std::visit([&](auto& conn)
         { return conn->databaseName(); }, _dbService->sqlConnPtrVariant() );

   std::string appDbInfo = dbName + " " + dbInformation;

   auto startPoint    = appStatus.startedTime->timePoint();
   auto nowPoint      = DateTime::now().timePoint();
   auto interval      = ( nowPoint - startPoint ).count();
   std::string uptime = util::readMilliseconds(interval);

   std::string runningState = "Running, since " + appStatus.startedTime->isoDateTimeString();

   std::string webappThreadIds;
   for (auto threadId: appStatus.webappThreadIds)
   {
      if (!webappThreadIds.empty())
         webappThreadIds += ", ";
      
      webappThreadIds += threadId;
   }

   Json currentConnections = Json::array();
   for (auto& info: appStatus.currentConnections )
   {
      Json conn;
      conn["connId"]         = info.connId;
      conn["remoteEndpoint"] = info.remoteEndpoint;
      conn["closed"]         = info.closed;
      conn["tls"]            = info.tls;
      conn["websocket"]      = info.websocket;
      conn["identifier"]     = info.identifier;
      conn["startTime"]      = info.startTime;
      conn["duration"]       = info.duration;

      currentConnections.emplace_back(std::move(conn));
   }

   Json resp = Json::array();

   resp.emplace_back( Json::array_t{0,  "App name",       appNameVersion } );
   resp.emplace_back( Json::array_t{1,  "App build date", appBuildDate } );
   resp.emplace_back( Json::array_t{2,  "App compiler",   appcompilerInfo } );
   resp.emplace_back( Json::array_t{3,  "App database",   appDbInfo } );
   resp.emplace_back( Json::array_t{4,  "Status",         runningState } );
   resp.emplace_back( Json::array_t{5,  "Uptime",         uptime } );
   resp.emplace_back( Json::array_t{6,  "HTTPS Only",     appStatus.httpsOnly } );
   resp.emplace_back( Json::array_t{7,  "HTTP Port",      appStatus.httpPort } );
   resp.emplace_back( Json::array_t{8,  "HTTPS Port",     appStatus.httpsPort } );

   resp.emplace_back( Json::array_t{9,  "Process Id",                appStatus.processId } );
   resp.emplace_back( Json::array_t{10, "Main thread id",            appStatus.appThreadId } );
   resp.emplace_back( Json::array_t{11, "Webapp thread pool size",   appStatus.threadpoolSize } );
   resp.emplace_back( Json::array_t{12, "Webapp thread ids",         webappThreadIds } );

   resp.emplace_back( Json::array_t{13, "Webapp total http client",        appStatus.totalHttpConnection } );
   resp.emplace_back( Json::array_t{14, "Webapp total https client",       appStatus.totalHttpsConnection } );
   resp.emplace_back( Json::array_t{15, "Total http connection created",   appStatus.lastHttpConnectionId } );
   resp.emplace_back( Json::array_t{16, "Total https connection created",  appStatus.lastHttpsConnectionId } );
   resp.emplace_back( Json::array_t{17, "Connections",                     currentConnections } );
   resp.emplace_back( Json::array_t{18, "Main Database Status",            (appStatus.dbConnected ? "Connected":"Not connected") } );
   resp.emplace_back( Json::array_t{19, "Errors",                          appStatus.errors } );

   bool buildWithInMemoryRes = false;
#ifdef TOBASA_BUILD_IN_MEMORY_RESOURCES
   buildWithInMemoryRes = true;
#endif   
   resp.emplace_back( Json::array_t{20, "Build with in-memory resources",  buildWithInMemoryRes } );
   resp.emplace_back( Json::array_t{21, "Using in-memory resources",       web::conf::Webapp::useInMemoryResources } );



   bool builWithInMemoryTZDB = false;
#if !defined(TOBASA_USE_STD_DATE) && defined(TOBASA_DATE_USE_IN_MEMORY_TZDB)
   builWithInMemoryTZDB = true;
#endif 
   resp.emplace_back( Json::array_t{22, "Build with in-memory Timezone DB", builWithInMemoryTZDB } );
   resp.emplace_back( Json::array_t{23, "Using in-memory Timezone DB",      DateTime::usingInMemoryTZDB() } );


   resp.emplace_back( Json::array_t{24, "Web Root",      app::docRoot() } );
   resp.emplace_back( Json::array_t{25, "Tempate Dir",   app::templateDir() } );
   resp.emplace_back( Json::array_t{26, "Data Dir",      app::dataDir() } );
   resp.emplace_back( Json::array_t{27, "Upload Dir",    app::uploadDir() } );



   return web::object(resp);
}


//! Handle GET request to /api/running_configuration
http::ResultPtr ApiCoreController::onGetAppConfig(const web::RouteArgument& arg)
{
   auto httpCtx = arg.httpContext();
   auto& authResult = std::any_cast<web::AuthResult&>( httpCtx->userData() );

   if (authResult.identity.pUser->userName != "admin")
      return web::forbidden();

   // get nlohmann json object
   auto& jsconConf = Config::get().getConfiguration();
   Json response;
   response["config"] = jsconConf;//jsconConf.dump(3);
   return web::object(response);
}


} // namespace web
} // namespace tbs