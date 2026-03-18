#pragma once

#include <tobasa/non_copyable.h>
#include <tobasasql/sql_conn_variant.h>
#include <tobasasql/database_service_factory_base.h>
#include "tobasaweb/controller_factory_base.h"
#include "tobasaweb/middleware_factory_base.h"
#include "tobasaweb/router_factory_base.h"
#include "tobasahttp/server/common.h"
#include <asio/io_context.hpp>

namespace tbs {
namespace web {

/** \addtogroup WEB
 * @{
 */

/** 
 * WebServiceBase.
 */
class WebServiceBase : private NonCopyable
{
public:
   WebServiceBase() = default;
   virtual ~WebServiceBase() = default;

   virtual std::shared_ptr<RouterBase>   router()            = 0;
   virtual asio::io_context&             ioContext()         = 0;

   virtual void useDbService(  sql::DbServiceFactoryPtr dbService)             = 0;
   virtual void useRouter(     std::shared_ptr<RouterFactoryBase> factory)     = 0;
   virtual void addController( std::shared_ptr<ControllerFactoryBase> factory) = 0;
   virtual void addMiddleware( std::shared_ptr<MiddlewareFactoryBase> factory) = 0;
   virtual void setupHandlers()                                                = 0;
   virtual std::unique_ptr<http::RequestHandler> serverHttpRequestHandler()    = 0;
   virtual void setIoContext(asio::io_context& ioContext)                      = 0;

   virtual void serverStatusPageBuilder(http::StatusPageBuilder renderer)      = 0;
   virtual http::StatusPageBuilder serverStatusPageBuilder()                   = 0;

   virtual void reconfigureDb(sql::DbServiceFactoryPtr dbService) = 0;

   virtual sql::DbServiceFactoryPtr dbServiceFactory() = 0;

};

using WebServicePtr = std::shared_ptr<WebServiceBase>;

/** @}*/

} // namespace web
} // namespace tbs
