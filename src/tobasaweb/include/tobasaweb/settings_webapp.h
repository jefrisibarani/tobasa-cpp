#pragma once

#include <tobasa/json.h>
#include <tobasasql/settings.h>
#include "tobasaweb/settings_http_server.h"

namespace tbs {
namespace web {

/// Configuration option classes
namespace conf {

struct RouteSession
{
   std::string path;
   std::string check;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RouteSession, path, check)

struct RouteAuth
{
   std::string path;
   std::string check;
   std::string authScheme;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RouteAuth, path, check, authScheme)

struct RouteAuthLists
{
   std::vector<RouteAuth> noAuthenticationList;
   std::vector<RouteAuth> needAuthenticationList;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RouteAuthLists,  noAuthenticationList, needAuthenticationList)

struct WebService
{
   RouteAuthLists routeAuthLists;

   // The number of MINUTES the session to last.
   // Setting to 0 (zero) means expire when the browser is closed.
   int sessionExpirationMinutes     {15};

   std::string sessionSavePath      {"./appdata/session"}; // Do not include trailing slash

   /// Client app ID accepted by this server
   std::string acceptedClientAppId  {"TBSRESTC_DEV,TBSRESTC_TOBASA"}; // comma separated values

   /// JWT Token Issuer
   std::string authJwtIssuer        {"TBS_WEBSVC"};

   /// JWT Access Token Secret
   std::string authJwtSecret        {"C4BC3A3AC2D6D367A74580388B20BC069C96B048DFEAF5CCDC0CE1E25BF23F39BBBKOKLPMKOJXXXLITWOERTYCBNVCXJBOAI49540KDHGASGIAHETGIDOGHOZ0E9RUIHGAHOIHDA"};
   
   /// JWT Refresh Token Secret
   std::string authJwtSecretRefresh {"C4BC3A3AC2D6D367A74580388B20BC069C96B048DFEAF5CCDC0CE1E25BF23F3"};

   /// JWT Access Token expired time in minutes
   int authJwtExpireTimeSpanMinutes        {15};     // 15 minutes
   
   /// JWT Refresh Token expired time in minutes
   int authJwtRefreshExpireTimeSpanMinutes {24*60};  // 24 hours
   
   /// Wether to use compiled wwwroot, resources and templates
   bool useInMemoryResources {true};

   /// View template directory
   std::string templateDir  {"./views"};             // Do not include trailing slash
   /// Upload directory
   std::string uploadDir    {"./appdata/upload"};   // Do not include trailing slash
   /// Data directory
   std::string dataDir      {"./appdata"};           // Do not include trailing slash

   std::string homePage     {"/dashboard"};
   std::string loginPage    {"/login"};
   std::string logoutPage   {"/logout"};

   std::vector<RouteSession> noSessionList;
 
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(WebService, routeAuthLists, sessionExpirationMinutes, 
   sessionSavePath, acceptedClientAppId, authJwtIssuer, authJwtSecret, authJwtSecretRefresh,
   authJwtExpireTimeSpanMinutes, authJwtRefreshExpireTimeSpanMinutes, 
   useInMemoryResources, templateDir, uploadDir, dataDir, homePage, loginPage, logoutPage, noSessionList)

struct Webapp
{
   sql::conf::ConnectorOption dbConnection;
   http::conf::Server         httpServer;
   WebService                 webService;
   size_t                     dbConnectionPoolSize;

   inline static bool useInMemoryResources {true};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Webapp, 
   dbConnection, httpServer, webService, dbConnectionPoolSize)

} // namespace conf
} // namespace web
} // namespace tbs
