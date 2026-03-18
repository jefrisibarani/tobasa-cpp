#pragma once

#include <tobasahttp/server/common.h>
#include <tobasahttp/response.h>
#include <tobasahttp/authentication.h>
#include "tobasaweb/middleware.h"
#include "tobasaweb/router_base.h"
#include "tobasaweb/settings_webapp.h"

namespace tbs {
namespace web {

/** 
 * \ingroup WEB
 * RouteEntry.
 * Router route/path handler and matcher
 */
class RouteEntry
{
private:
   http::Method      _method {http::Method::UNKNOWNN};
   std::string       _entryPath;
   RouteHandler      _handler;
   http::AuthScheme  _authScheme;
   std::string       _matchRule;

   int               _id = 0;
   inline static int _counter = 0;
   
public:
   RouteEntry();
   RouteEntry(http::Method method, 
      const std::string& path, 
      RouteHandler handler, 
      http::AuthScheme authScheme = http::AuthScheme::NONE,
      const std::string& matchRule = "");

   int id();
   http::Method method() const;
   std::string path() const;
   RouteHandler handler() const;
   http::AuthScheme authScheme() const;
   void method(http::Method method);
   void path(const std::string& path);
   void authScheme(http::AuthScheme scheme);
   std::string matchRule() const;
   void matchRule(const std::string rule);

   /// Set router handler
   void handler(RouteHandler handler);

   /// Invoke route handler
   std::shared_ptr<http::Result> invokeHandler(RouteArgument && routeArg);

   /// Inspect route entry and request path
   // check for match:  /version/{id}/build/{name} with /version/909/build/jhon
   virtual bool canHandlePath(const http::HttpContext& context);
};


/** 
 * \ingroup WEB
 * Router
 * Web application router and HTTP server request handler
 */
class Router
   : public Middleware
   , public RouterBase
{
protected:
   std::vector<RouteEntry> _routeEntries;

   // Acts as the default handler for routes that do not have
   // a specific handler registered in _routeEntries
   RouteHandler _defaultHandler;

public:
   Router();
   virtual ~Router() = default;

   virtual std::string name();

   virtual void httpGet(const std::string& path, RouteHandler&& handler, http::AuthScheme authScheme=http::AuthScheme::NONE, const std::string& matchRule="");
   virtual void httpPost(const std::string& path, RouteHandler&& handler, http::AuthScheme authScheme=http::AuthScheme::NONE, const std::string& matchRule="");
   virtual void httpPut(const std::string& path, RouteHandler&& handler, http::AuthScheme authScheme=http::AuthScheme::NONE, const std::string& matchRule="");
   virtual void httpDelete(const std::string& path, RouteHandler&& handler, http::AuthScheme authScheme=http::AuthScheme::NONE, const std::string& matchRule="");

   virtual void defaultHandler(RouteHandler&& handler);

   // Check HTTP request path handler and authentication type,
   // set up HTTP context route handler and authentication scheme,
   // apply routing rule from configuration file (appsettings.json),
   // initialize and set up route's AuthContext.
   // This function is called in MiddlewareManager::invoke()
   virtual void setupRoute(const http::HttpContext& context);

   http::RequestStatus invoke(const http::HttpContext& context);

protected:
   /// Do some initialization if needed
   /// Factory call this method in initRouter()
   virtual void onInit();

   void addHandler(http::Method method, const std::string& path, RouteHandler&& handler, http::AuthScheme authScheme=http::AuthScheme::NONE, const std::string& matchRule="");
   
   conf::RouteAuth readAuthConfigurationRule(const std::vector<conf::RouteAuth>& rules, const std::string& requestPath);


   // Check authentication requirements from the configuration file
   // and determine the effective authentication scheme.
   void setupAuthenticationRule(const http::HttpContext& context);   


   /**
    * \brief Reads authentication rules from the configuration.
    * \param requestPath The HTTP request path used to determine the authentication rule.
    * \param authScheme Output parameter to store the retrieved AuthScheme value
    */
   bool authenticationRequiredInConfigurationFile(const std::string& requestPath, http::AuthScheme& authScheme);

   /**
    * \brief Checks if authentication is disabled in the configuration file.
    * \param requestPath The HTTP request path used to determine if authentication is disabled.
    * \param authScheme Output parameter to store the retrieved AuthScheme value
    * \return A boolean indicating if authentication is disabled
    */
   bool authenticationDisabledInConfigurationFile(const std::string& requestPath, http::AuthScheme& authScheme);

private:

   std::vector<conf::RouteAuth> _noAuthenticationList;
   std::vector<conf::RouteAuth> _needAuthenticationList;

};

} // namespace web
} // namespace tbs