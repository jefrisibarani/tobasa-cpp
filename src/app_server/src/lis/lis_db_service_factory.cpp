#include <tobasasql/database_connector.h>
#include "lis_db_service_factory.h"
#include "lis_db_service_base.h"
#include "lis_db_service_hl7.h"
#include "lis_db_service_lis2a.h"
#include "lis_db_service_lis2a_devtest.h"
#include "lis_db_service_lis2a_indiko.h"
#include "lis_db_service_lis2a_dxh500.h"
#include "lis_db_service_dirui.h"
#include "lis_db_service_vitek2_compact.h"
#include "lis_db_service_vidas.h"
#include "lis_db_service_hl7_devtest.h"

namespace tbs {
namespace lis {

DbServiceFactory::DbServiceFactory(const sql::conf::ConnectorOption& connOpt, const std::string& instrumentType)
   : _connOption(connOpt)
   , _instrumentType(instrumentType)
{}

DbServiceFactory::~DbServiceFactory()
{
   for (auto connector: _connectorList)
   {
      connector->disconnect();
      delete connector;
      connector = nullptr;
   }
}

svc::LisServicePtr DbServiceFactory::lisService(bool newConnection)
{
   sql::DatabaseConnector* conn = getConnector(newConnection);
   svc::LisServicePtr repo = createLisService(conn);
   return repo;
}

svc::LisHL7ServicePtr DbServiceFactory::lisHL7Service(bool newConnection)
{
   svc::LisHL7ServicePtr realSvc = nullptr;
   svc::LisServicePtr svc = lisService(newConnection);

   if (svc) {
      realSvc = std::static_pointer_cast<svc::LisHL7ServiceBase>(svc);
   }

   if (realSvc == nullptr)
      throw AppException("Could not create Lis DB Service");

   return realSvc;
}

svc::Lis2aServicePtr DbServiceFactory::lis2aService(bool newConnection)
{
   svc::Lis2aServicePtr realSvc = nullptr;
   svc::LisServicePtr svc = lisService(newConnection);

   if (svc) {
      realSvc = std::static_pointer_cast<svc::Lis2aServiceBase>(svc);
   }

   if (realSvc == nullptr)
      throw AppException("Could not create Lis DB Service");

   return realSvc;
}

svc::LisServicePtr DbServiceFactory::createLisService(sql::DatabaseConnector* connector)
{
   if ( connector != nullptr && connector->connected())
   {
      // LIS1A Devices
      if ( _instrumentType == lis::DEV_DEFAULT_LIS1A)
      {
         auto service = getConnector(false)->createService<lis::svc::Lis2aServiceDevTest>();
         auto real = std::static_pointer_cast<lis::svc::LisServiceBase>(service);
         return real;
      }
      else if ( _instrumentType == lis::DEV_TEST_LIS1A)
      {
         auto service = getConnector(false)->createService<lis::svc::Lis2aServiceDevTest>();
         auto real = std::static_pointer_cast<lis::svc::LisServiceBase>(service);
         return real;
      }
      else if ( _instrumentType == lis::DEV_INDIKO)
      {
         auto service = getConnector(false)->createService<lis::svc::Lis2aServiceIndiko>();
         auto real = std::static_pointer_cast<lis::svc::LisServiceBase>(service);
         return real;
      }
      else if ( _instrumentType == lis::DEV_DXH_500)
      {
         auto service = getConnector(false)->createService<lis::svc::Lis2aServiceDxH500>();
         auto real = std::static_pointer_cast<lis::svc::LisServiceBase>(service);
         return real;
      }
      else if ( _instrumentType == lis::DEV_GEM_3500  || _instrumentType == lis::DEV_SELECTRA)
      {
         auto service = getConnector(false)->createService<lis::svc::Lis2aService>();
         auto real = std::static_pointer_cast<lis::svc::LisServiceBase>(service);
         return real;
      }

      // HL7 Devices
      else if ( _instrumentType == lis::DEV_DEFAULT_HL7)
      {
         auto service = getConnector(false)->createService<lis::svc::LisHL7ServiceDevTest>();
         auto real = std::static_pointer_cast<lis::svc::LisServiceBase>(service);
         return real;
      }
      else if ( _instrumentType == lis::DEV_TEST_HL7)
      {
         auto service = getConnector(false)->createService<lis::svc::LisHL7ServiceDevTest>();
         auto real = std::static_pointer_cast<lis::svc::LisServiceBase>(service);
         return real;
      }

      // BCI Devices
      else if ( _instrumentType == lis::DEV_VIDAS )
      {
         auto service = getConnector(false)->createService<lis::svc::LisVidasService>();
         auto real = std::static_pointer_cast<lis::svc::LisServiceBase>(service);
         return real;
      }
      else if ( _instrumentType == lis::DEV_VITEK2_COMPACT )
      {
         auto service = getConnector(false)->createService<lis::svc::LisVitek2Service>();
         auto real = std::static_pointer_cast<lis::svc::LisServiceBase>(service);
         return real;
      }

      // DIRUI Devices
      else if ( _instrumentType == lis::DEV_DIRUI_H_500 || _instrumentType == lis::DEV_DIRUI_BCC_3600)
      {
         auto service = getConnector(false)->createService<lis::svc::LisDirUIService>();
         auto real = std::static_pointer_cast<lis::svc::LisServiceBase>(service);
         return real;
      }

      else
      {
         throw AppException("Unsupported LIS instrument type");
      }
   }
   else
      throw std::runtime_error("No connection to database");
}

sql::DatabaseConnector* DbServiceFactory::getConnector(bool newConnection)
{
   if (newConnection)
   {
      return newConnector();
   }
   else 
   {
      sql::DatabaseConnector* conn = nullptr;
      // create first connection
      if (_connectorList.size() == 0) {
         conn = newConnector();
      }
      else {
         conn = _connectorList[0];
      }
  
      return conn;
   }
}

sql::DatabaseConnector* DbServiceFactory::newConnector()
{
   // Create new DatabaseConnector instance
   sql::DatabaseConnector* conn = new sql::DatabaseConnector(_connOption, "LisDbService") ;
   conn->initSqlDriver();
   conn->connect();

   _connectorList.push_back(conn);

   return conn;
}

} // namespace lis
} // namespace tbs