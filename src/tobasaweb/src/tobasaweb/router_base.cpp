#include <tobasahttp/server/common.h>
#include <tobasahttp/request.h>
#include <tobasahttp/methods.h>
#include "tobasaweb/router_factory.h"
#include "tobasaweb/router_base.h"

namespace tbs {
namespace web {

RouteArgument::RouteArgument(const http::HttpContext& context, const std::string& pathTemplate)
   : _httpContext(context)
{
   _pathArguments = _httpContext->request()->pathArgument(pathTemplate);
}

http::HttpContext RouteArgument::httpContext() const
{
   return _httpContext;
}

std::optional<std::string> RouteArgument::get(const std::string& name) const
{
   if (_pathArguments)
      return _pathArguments->get(name);
   else
      return std::nullopt;
}

std::optional<ArgumentDataPtr> RouteArgument::get(int position) const
{
   if (_pathArguments) 
      return _pathArguments->get(position);
   else
      return std::nullopt;      
}

} // namespace http
} // namespace tbs
