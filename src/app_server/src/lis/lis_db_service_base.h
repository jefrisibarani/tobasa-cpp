#pragma once

#include <tobasa/datetime.h>
#include <tobasasql/sql_service_base.h>
#include <tobasalis/lis/message.h>

namespace tbs {
namespace lis {
namespace svc {

struct DbStoreResult
{
   int64_t     headerId = 0;
   /// lis::Message internal ID
   std::string internalMessageId;  
};  
using DbStoreResultPtr = std::shared_ptr<DbStoreResult>;

struct MessageHttpUploadStatus
{
   std::string instrumentType;
   int64_t     headerId = 0;
   std::string startTime;
   std::string responseTime;
   std::string status;

   /// lis::Message internal ID
   std::string internalMessageId;
};

class LisServiceBase : public sql::SqlServiceBase
{
public:
   LisServiceBase() = default;
   virtual ~LisServiceBase() = default;
   virtual bool saveLisData(const lis::MessagePtr& message, DbStoreResultPtr dbStoreResult=nullptr) = 0;
   virtual std::string dbVersionString() = 0;

protected:

   virtual std::string getDateTime(const std::string& lisDateTime)
   {
      if (!lisDateTime.empty())
      {
         if (lisDateTime.length() == 14)
         {
            tbs::DateTime dt;
            // 20240105153444
            if (dt.parse(lisDateTime, "%Y%m%d%H%M%S"))
            {
               std::string datetime = dt.isoDateTimeString();
               return datetime;
            }
            else
               throw AppException("Invalid date time input string");
         }
         else if (lisDateTime.length() == 8)
         {
            tbs::DateTime dt;
            // 20240105
            if (dt.parse(lisDateTime, "%Y%m%d"))
            {
               std::string datetime = dt.isoDateString();
               datetime += " 00:00:00";
               return datetime;
            }
            else
               throw AppException("Invalid date time input string");
         }         

      }

      return {};
   }

   virtual std::string getDate(const std::string& lisDateTime)
   {
      if (!lisDateTime.empty())
      {
         if (lisDateTime.length() == 14)
         {
            tbs::DateTime dt;
            // 20240105153444
            if (dt.parse(lisDateTime, "%Y%m%d%H%M%S"))
            {
               std::string datetime = dt.isoDateString();
               return datetime;
            }
            else
               throw AppException("Invalid date time input string");
         }
         else if (lisDateTime.length() == 8)
         {
            tbs::DateTime dt;
            // 20240105
            if (dt.parse(lisDateTime, "%Y%m%d"))
            {
               std::string datetime = dt.isoDateString();
               return datetime;
            }
            else
               throw AppException("Invalid date time input string");
         }         
      }

      return {};
   }

   void throwIfNotSupportedBackend(sql::BackendType backendType)
   {
      if (! (backendType == sql::BackendType::pgsql  ||
             backendType == sql::BackendType::sqlite ||
             backendType == sql::BackendType::adodb  ||
             backendType == sql::BackendType::mysql  ||
             backendType == sql::BackendType::odbc ) )
      {
         throw AppException("Unsupported database backend");
      }
   }
};

using LisServicePtr = std::shared_ptr<LisServiceBase>;

} // namespace svc
} // namespace lis
} // namespace tbs


