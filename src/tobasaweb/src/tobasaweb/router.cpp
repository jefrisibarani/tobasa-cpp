#include <tobasa/config.h>
#include <tobasa/util_string.h>
#include <tobasahttp/exception.h>
#include "tobasaweb/router.h"
#include "tobasaweb/cookie.h"


namespace tbs {
namespace web {

RouteEntry::RouteEntry() {}

RouteEntry::RouteEntry(
   http::Method method, 
   const std::string& path, 
   RouteHandler handler, 
   http::AuthScheme authScheme,
   const std::string& matchRule)
   : _method     { method }
   , _entryPath  { path }
   , _handler    { std::move(handler) }
   , _authScheme { authScheme}
   , _matchRule  { matchRule}
{
   ++_counter;
   _id = _counter;
}

int RouteEntry::id()
{
   return _id;
}

http::Method RouteEntry::method() const
{
   return _method;
}

std::string RouteEntry::path() const
{
   return _entryPath;
}

http::AuthScheme RouteEntry::authScheme() const
{
   return _authScheme;
}

void RouteEntry::authScheme(http::AuthScheme scheme)
{
   _authScheme = scheme;
}

std::string RouteEntry::matchRule() const
{
   return _matchRule;
}

void RouteEntry::matchRule(const std::string rule)
{
   _matchRule = rule;
}

RouteHandler RouteEntry::handler() const
{
   return _handler;
}

void RouteEntry::method(http::Method method)
{
   _method = method;
}

void RouteEntry::path(const std::string& path)
{
   if (path.empty())
      throw http::Exception("Invalid route entry path argument");

   std::string_view vpath {path};
   // remove an optional trailing slash.
   if (vpath.size() > 1u && '/' == vpath.back())
      vpath.remove_suffix(1);

   _entryPath = vpath;
}

void RouteEntry::handler(RouteHandler handler)
{
   _handler =  std::move(handler);
}

std::shared_ptr<http::Result> RouteEntry::invokeHandler(RouteArgument && routeArg)
{
   return _handler(std::move(routeArg));
}

bool RouteEntry::canHandlePath(const http::HttpContext& context)
{
   if ( _method != http::httpMethodFromString( context->request()->method()) )
      return false;

   std::string reqPath = context->request()->path();

   if (reqPath.empty() || _entryPath.empty())
      return false;

   auto pathArg = context->request()->pathArgument(_entryPath);
   if ( pathArg && pathArg->match())
      return true;
   else if (_matchRule == "starts_with" && util::startsWith(reqPath, _entryPath) )
      return true;


   return false;
}

Router::Router() {}

void Router::onInit()
{
   // at the time this function called by factory, Config already loaded

   auto appOption = Config::getOption<web::conf::Webapp>("webapp");
   _noAuthenticationList   = appOption.webService.routeAuthLists.noAuthenticationList;
   _needAuthenticationList = appOption.webService.routeAuthLists.needAuthenticationList;
}

std::string Router::name()
{
   return "Router";
}

void Router::httpGet(const std::string& path, RouteHandler&& handler, http::AuthScheme authScheme, const std::string& matchRule)
{
   addHandler(http::Method::GET, path, std::move(handler), authScheme, matchRule);
}

void Router::httpPost(const std::string& path, RouteHandler&& handler, http::AuthScheme authScheme, const std::string& matchRule)
{
   addHandler(http::Method::POST, path, std::move(handler), authScheme, matchRule);
}

void Router::httpPut(const std::string& path, RouteHandler&& handler, http::AuthScheme authScheme, const std::string& matchRule)
{
   addHandler(http::Method::PUT, path, std::move(handler), authScheme, matchRule);
}

void Router::httpDelete(const std::string& path, RouteHandler&& handler, http::AuthScheme authScheme, const std::string& matchRule)
{
   addHandler(http::Method::DEL, path, std::move(handler), authScheme, matchRule);
}

void Router::defaultHandler(RouteHandler&& handler)
{
   _defaultHandler = std::move(handler);
}

void Router::setupRoute(const http::HttpContext& context)
{
   for (auto& entry: _routeEntries)
   {
      if (entry.canHandlePath(context))
      {
         context->requestHandlerId(entry.id());
         context->request()->authContext()->definedScheme = entry.authScheme();

         // calculate auth rule for this matching route
         setupAuthenticationRule(context);
         return;
      }
   }

   // also calculate other route not registered in _routeEntries
   setupAuthenticationRule(context);
   return;
}

http::RequestStatus Router::invoke(const http::HttpContext& context)
{
   auto response = context->response();

   for (auto& entry: _routeEntries)
   {
      if ( entry.id() == context->requestHandlerId() )
      {
         Logger::logT("[webapp] Router executing handler, entry id: {}, requestHandlerId: {}", entry.id(), context->requestHandlerId());

         RouteArgument routeArgument(context, entry.path());

         auto result = entry.invokeHandler( std::move(routeArgument) );
         if (_resultContentBuilderMap.find(result->className()) != _resultContentBuilderMap.end() )
         {
            auto builder = _resultContentBuilderMap[result->className()];
            result->toResponse(response, builder);
         }
         else {
            result->toResponse(response);
         }

         return http::RequestStatus::handled;
      }
   }

   if (_defaultHandler)
   {
      auto result = _defaultHandler( RouteArgument(context) );

      if (_resultContentBuilderMap.find(result->className()) != _resultContentBuilderMap.end() )
      {
         auto builder = _resultContentBuilderMap[result->className()];
         result->toResponse(response,builder);
      }
      else {
         result->toResponse(response);
      }

      Logger::logT("[webapp] Router executing default handler");
      return http::RequestStatus::handled;
   }
   else
   {
      Logger::logT("[webapp] Router executing built-in default handler");
      // built-in default handler
      http::StatusResult result(http::StatusCode::NOT_FOUND);

      if (_resultContentBuilderMap.find(result.className()) != _resultContentBuilderMap.end() )
      {
         auto builder = _resultContentBuilderMap[result.className()];
         result.toResponse(response,builder);
      }
      else {
         result.toResponse(response);
      }

      return http::RequestStatus::handled;
   }

   return http::RequestStatus::notHandled;
}


void Router::addHandler(http::Method method, const std::string& path, RouteHandler&& handler, http::AuthScheme authScheme, const std::string& matchRule)
{
   if (path.empty())
      throw http::Exception("Invalid route entry path argument");

   std::string_view vpath {path};
   // remove an optional trailing slash.
   if (vpath.size() > 1u && '/' == vpath.back())
      vpath.remove_suffix(1);

   Logger::logT("[webapp] [route] addHandler, method: {}, path: {}", httpMethodToString(method), path);
   _routeEntries.emplace_back( method, std::string{vpath}, std::move(handler), authScheme, matchRule);
}


conf::RouteAuth Router::readAuthConfigurationRule(const std::vector<conf::RouteAuth>& rules, const std::string& requestPath)
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


// Check authentication requirements from the configuration file
// and determine the effective authentication scheme.
void Router::setupAuthenticationRule(const http::HttpContext& context)
{
   bool disabled = false, required = false, skipChecking = false;
   http::AuthScheme routeAuthSchemeDisabled = http::AuthScheme::NONE;
   http::AuthScheme routeAuthSchemeRequired = http::AuthScheme::NONE;
   
   disabled = authenticationDisabledInConfigurationFile(context->request()->path(), routeAuthSchemeDisabled);
   // If authentication is required, it takes precedence over being disabled
   required = authenticationRequiredInConfigurationFile(context->request()->path(), routeAuthSchemeRequired);
  
   skipChecking = disabled && (!required);
   auto authContext = context->request()->authContext();
   authContext->disableCheck = skipChecking;

   if (! skipChecking)
   {
      // authentication for this request path is required in configuration file
      if ( routeAuthSchemeRequired != http::AuthScheme::NONE)
         authContext->effectiveScheme = routeAuthSchemeRequired;      // scheme from configuration has priority
      else if ( authContext->definedScheme != http::AuthScheme::NONE)
         authContext->effectiveScheme = authContext->definedScheme;   // then from registered scheme in controllers registration
      else
         authContext->effectiveScheme = authContext->scheme;          // last, get from parsed scheme in request header
   }
   else
   {
      // authentication requirement not listed in configuration file
      authContext->effectiveScheme = authContext->definedScheme;
   }
}


/** Read authentication rule from configuration
 * @param requestPath http request path
 * @param authScheme  (out) AuthScheme value defined in configuration
 */
bool Router::authenticationDisabledInConfigurationFile(const std::string& requestPath, http::AuthScheme& authScheme)
{
   auto rule = readAuthConfigurationRule(_noAuthenticationList, requestPath);
   if (rule.path.empty())
      return false;
   
   authScheme = http::authSchemeFromString(rule.authScheme);
   return true;
}


/** Read authentication rule from configuration
 * @param requestPath http request path
 * @param authScheme  (out) AuthScheme value defined in configuration
 */
bool Router::authenticationRequiredInConfigurationFile(const std::string& requestPath, http::AuthScheme& authScheme)
{
   auto rule = readAuthConfigurationRule(_needAuthenticationList, requestPath);
   if (rule.path.empty())
      return false;
   
   authScheme = http::authSchemeFromString(rule.authScheme);
   return true;
}

} // namespace web
} // namespace tbs