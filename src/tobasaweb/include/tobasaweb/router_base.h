#pragma once

#include <map>
#include <tobasahttp/server/common.h>
#include <tobasahttp/request.h>
#include <tobasahttp/methods.h>
#include "tobasaweb/router_factory.h"
#include "tobasaweb/result.h"

namespace tbs {
namespace web {

/** \addtogroup WEB
 * @{
 */


/** 
 * Path argument property.
 * position start from 0
 */
using ArgumentData    = http::Field;
using ArgumentDataPtr = http::FieldPtr;

class RouteEntry;

/** 
 * Route handler argument store
 */
class RouteArgument
{
private:
   friend class RouteEntry;
   const http::HttpContext _httpContext;
   http::PathArgumentPtr   _pathArguments;

public:
   RouteArgument(const http::HttpContext& context, const std::string& pathTemplate="");
   http::HttpContext httpContext() const;
   std::optional<std::string> get(const std::string& name) const;
   std::optional<ArgumentDataPtr> get(int position) const;
};

using RouteHandler = std::function<std::shared_ptr<http::Result>(const RouteArgument&) >;

/** 
 * Router base class
 */
class RouterBase
{
protected:
   /// Factory to manage this router
   template<class RouterImpl> friend class RouterFactory;
   std::map<std::string, http::ResultContentBuilder> _resultContentBuilderMap;  

public:
   RouterBase(const RouterBase &) = delete;
   RouterBase(RouterBase &&) = delete;
   RouterBase() = default;
   virtual ~RouterBase() = default;

   virtual void httpGet(   const std::string& path, RouteHandler&& handler, http::AuthScheme authScheme=http::AuthScheme::NONE, const std::string& matchRule="") = 0;
   virtual void httpPost(  const std::string& path, RouteHandler&& handler, http::AuthScheme authScheme=http::AuthScheme::NONE, const std::string& matchRule="") = 0;
   virtual void httpPut(   const std::string& path, RouteHandler&& handler, http::AuthScheme authScheme=http::AuthScheme::NONE, const std::string& matchRule="") = 0;
   virtual void httpDelete(const std::string& path, RouteHandler&& handler, http::AuthScheme authScheme=http::AuthScheme::NONE, const std::string& matchRule="") = 0;
   
   virtual void defaultHandler(RouteHandler&& handler) = 0;

   virtual std::string name() = 0;

   // Check htttp request path handler and authentication type,
   // setup http context route handler and authentication scheme
   // This function called in MiddlewareManager::invoke()
   virtual void setupRoute(const http::HttpContext& context) = 0;

   void addResultContentBuilder(http::ResultContentBuilder builder, const std::string& resultClassName)
   {
      _resultContentBuilderMap[resultClassName] = std::move(builder);
   }

   

protected:

   /// Do some initialization if needed
   /// Factory call this method in initRouter()
   virtual void onInit() {}

};

using RouterPtr = std::shared_ptr<RouterBase>;

/** @}*/

} // namespace http
} // namespace tbs