#include "tobasaweb/service_client_base.h"

namespace tbs {
namespace web {

sql::DbServiceFactoryPtr ServiceClientBase::dbServiceFactory() 
{
   return _dbServiceFactory;
}

/// Factory call this method in initMiddleware()

void ServiceClientBase::setDbServiceFactory(sql::DbServiceFactoryPtr factory)
{
   _dbServiceFactory = factory;
}


} // namespace web
} // namespace tbs