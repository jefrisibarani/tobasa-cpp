#pragma once

#include "tobasaweb/exp_imp.h"

#include <tobasasql/sql_conn_variant.h>
#include <tobasasql/database_service_factory_base.h>

namespace tbs {
namespace web {

/** 
 * \ingroup WEB
 * ServiceClientBase.
 * Class providing access to AuthDbRepo and SqlConnection
 */
class TBS_WEB_API ServiceClientBase
{
public:
   ServiceClientBase(const ServiceClientBase &) = delete;
   ServiceClientBase(ServiceClientBase &&) = delete;
   ServiceClientBase() = default;
   virtual ~ServiceClientBase() = default;

protected:
   sql::DbServiceFactoryPtr _dbServiceFactory;

public:
   virtual sql::DbServiceFactoryPtr dbServiceFactory();

protected:

   /// Factory call this method in initMiddleware()
   virtual void setDbServiceFactory(sql::DbServiceFactoryPtr factory);
};

} // namespace web
} // namespace tbs