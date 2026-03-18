#pragma once

#include <tobasalis/lis/common.h>
#include <tobasalis/bci/message.h>
#include "lis_db_service_tempate.h"

namespace tbs {
namespace lis {
namespace svc {

template <typename SqlDriverType>
class LisVitek2Service : public LisServiceTemplate<SqlDriverType>
{
public:
   using SqlResult     = sql::SqlResult<SqlDriverType>;
   using SqlConnection = sql::SqlConnection<SqlDriverType>;
   using SqlQuery      = sql::SqlQuery<SqlDriverType>;
   using Helper        = typename SqlDriverType::HelperImpl;
   using SqlApplyLogId = sql::SqlApplyLogId<SqlDriverType>;

private:
   LisVitek2Service(const LisVitek2Service &) = delete;
   LisVitek2Service(LisVitek2Service &&) = delete;

public:
   LisVitek2Service() {}
   LisVitek2Service(SqlConnection& conn) 
      : LisServiceTemplate<SqlDriverType>{ conn }  {}

   virtual ~LisVitek2Service() {}

   virtual bool saveLisData(const lis::MessagePtr& message, DbStoreResultPtr dbStoreResult=nullptr) override
   {
      SqlApplyLogId applyLogId(this->_sqlConn, message->internalId());

      if (message->instrumentType() == lis::DEV_VITEK2_COMPACT)
      {
         // TODO_JEFRI: BEGIN TRANSACTION
         auto bciMessage = std::static_pointer_cast<bci::BciMessage>(message);
         if (doSaveLisData(bciMessage, dbStoreResult))
         {
            // COMMIT TRANSACTION
            return true;
         }
         else
         {
            // ROLLBACK TRANSACTION
            return false;
         }
      }
      else
         throw AppException("Invalid protocol for LIS Vitek2 Compact service");

      return false;
   } 

protected:

   virtual bool doSaveLisData(const std::shared_ptr<bci::BciMessage>& message, DbStoreResultPtr dbStoreResult)
   {
      try
      {
         // TODO_JEFRI:
         // Save lis data here

         dbStoreResult->headerId = 0;
         dbStoreResult->internalMessageId = message->internalId();
         
         return true;
      }
      catch (const std::exception& ex)
      {
         Logger::logE("[lis_db] [msg:{}] Error occured saving LIS Vitek2 Compact data: {}", message->internalId(), ex.what());
      }

      return false;
   }      
};

} // namespace svc
} // namespace lis
} // namespace tbs