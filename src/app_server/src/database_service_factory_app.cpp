#include "database_service_factory_app.h"

namespace tbs {
namespace app {

DbServiceFactoryApp::DbServiceFactoryApp()
{
   Logger::logT("[app] DbServiceFactoryApp id: {} initialized", selfId());
}

DbServiceFactoryApp::~DbServiceFactoryApp()
{
   Logger::logT("[app] DbServiceFactoryApp id: {} destroyed", selfId());
}

sql::SqlServicePtr DbServiceFactoryApp::doGetDbService(const std::string& serviceName, bool pooled)
{
   if (serviceName == "AuthDbRepo")
      return createAuthDbRepo(pooled);
   else if (serviceName == "AppDbRepo")
      return createAppDbRepo(pooled);
   else if (serviceName == "UserAclDbRepo")
      return createUserAclDbRepo(pooled);
   else
      return nullptr;
}
 
web::AuthDbRepoPtr DbServiceFactoryApp::createAuthDbRepo(bool pooled)
{
   return std::static_pointer_cast<web::AuthDbRepoBase>(
      this->createService<web::AuthDbRepo>("MainAppDbOption", pooled)
   );
}

AppDbRepoPtr DbServiceFactoryApp::createAppDbRepo(bool pooled)
{
   return std::static_pointer_cast<AppDbRepoBase>(
      this->createService<AppDbRepo>("MainAppDbOption", pooled)
   );
}

UserAclDbRepoPtr DbServiceFactoryApp::createUserAclDbRepo(bool pooled)
{
   return std::static_pointer_cast<UserAclDbRepoBase>(
      this->createService<UserAclDbRepo>("MainAppDbOption", pooled)
   );
}

#ifdef TOBASA_USE_LIS_ENGINE
   lis::svc::LisServicePtr    DbServiceFactoryApp::lisService(bool pooled)
   {
      return nullptr;
   }

   lis::svc::LisHL7ServicePtr DbServiceFactoryApp::lisHL7Service(bool pooled)
   {
      return nullptr;
   }

   lis::svc::Lis2aServicePtr  DbServiceFactoryApp::lis2aService(bool pooled)
   {
      return nullptr;
   }

   lis::svc::LisServicePtr   DbServiceFactoryApp::createLisService(bool pooled)
   {
      return nullptr;
   }
#endif

} // namespace app
} // namespace tbs