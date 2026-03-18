#pragma once

#include <tobasa/self_counter.h>
#include <tobasasql/database_service_factory.h>
#include <tobasaweb/db_repo_auth.h>
#include "db_repo_user_acl.h"
#include "db_repo_app.h"

#ifdef TOBASA_USE_LIS_ENGINE
   #include "lis/lis_db_service_hl7.h"
   #include "lis/lis_db_service_lis2a.h"
#endif

namespace tbs {
namespace app {

class DbServiceFactoryApp : public sql::DbServiceFactory, public SelfCounter
{
public:
   DbServiceFactoryApp();
   ~DbServiceFactoryApp();

   virtual sql::SqlServicePtr doGetDbService(const std::string& serviceName, bool pooled = true);

   web::AuthDbRepoPtr createAuthDbRepo(bool pooled = true);
   AppDbRepoPtr createAppDbRepo(bool pooled = true);
   UserAclDbRepoPtr createUserAclDbRepo(bool pooled = true);

#ifdef TOBASA_USE_LIS_ENGINE
   lis::svc::LisServicePtr    lisService(bool pooled = true);
   lis::svc::LisHL7ServicePtr lisHL7Service(bool pooled = true);
   lis::svc::Lis2aServicePtr  lis2aService(bool pooled = true);
   lis::svc::LisServicePtr    createLisService(bool pooled = false);
#endif

};

using DbServicePtr = std::shared_ptr<DbServiceFactoryApp>;

} // namespace app
} // namespace tbs