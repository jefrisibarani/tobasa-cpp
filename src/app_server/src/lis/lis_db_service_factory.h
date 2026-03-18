#pragma once

#include <tobasasql/sql_driver.h>
#include <tobasasql/sql_conn_variant.h>
#include "lis_db_service_hl7.h"
#include "lis_db_service_lis2a.h"

namespace tbs {
namespace sql { class DatabaseConnector;}
namespace lis {

/**
 * \ingroup lis
 * DbServiceFactory
 */
class DbServiceFactory
{
private:
   DbServiceFactory( const DbServiceFactory & ) = delete;
   DbServiceFactory( DbServiceFactory && ) = delete;

protected:
   sql::conf::ConnectorOption _connOption;
   std::vector<sql::DatabaseConnector*> _connectorList;
   std::string _instrumentType;

public:
   DbServiceFactory(const sql::conf::ConnectorOption& connOpt, const std::string& instrumentType);
   virtual ~DbServiceFactory();

   svc::LisServicePtr    lisService(bool newConnection = false);
   svc::LisHL7ServicePtr lisHL7Service(bool newConnection = false);
   svc::Lis2aServicePtr  lis2aService(bool newConnection = false);

protected:
   sql::DatabaseConnector* getConnector(bool newConnection);
   sql::DatabaseConnector* newConnector();

   svc::LisServicePtr createLisService(sql::DatabaseConnector* connector);
};

} // namespace lis
} // namespace tbs


