#pragma once

#include <tobasalis/lis/common.h>
#include <tobasalis/bci/message.h>
#include "lis_db_service_tempate.h"

namespace tbs {
namespace lis {
namespace svc {

template <typename SqlDriverType>
class LisVidasService : public LisServiceTemplate<SqlDriverType>
{
public:
   using SqlResult     = sql::SqlResult<SqlDriverType>;
   using SqlConnection = sql::SqlConnection<SqlDriverType>;
   using SqlQuery      = sql::SqlQuery<SqlDriverType>;
   using Helper        = typename SqlDriverType::HelperImpl;
   using SqlApplyLogId = sql::SqlApplyLogId<SqlDriverType>;

private:
   LisVidasService(const LisVidasService &) = delete;
   LisVidasService(LisVidasService &&) = delete;

public:
   LisVidasService() {}
   LisVidasService(SqlConnection& conn) 
      : LisServiceTemplate<SqlDriverType>{ conn }  {}

   virtual ~LisVidasService() {}

   virtual bool saveLisData(const lis::MessagePtr& message, DbStoreResultPtr dbStoreResult=nullptr) override
   {
      SqlApplyLogId applyLogId(this->_sqlConn, message->internalId());

      if (message->instrumentType() == lis::DEV_VIDAS)
      {
         // TODO_JEFRI: BEGIN TRANSACTION
         auto vidasMessage = std::static_pointer_cast<bci::VidasMessage>(message);
         if (doSaveLisData(vidasMessage, dbStoreResult))
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
         throw AppException("Invalid protocol for LIS Vidas DB service");

      return false;
   }

protected:

   virtual bool doSaveLisData(const std::shared_ptr<tbs::bci::VidasMessage>& message, DbStoreResultPtr dbStoreResult)
   {
      try
      {
         return true;
      }
      catch (const std::exception& ex)
      {
         Logger::logE("[lis_db] [msg:{}] Error occured saving LIS Vidas data: {}", message->internalId(), ex.what());
      }

      return false;
   }
};

} // namespace svc
} // namespace lis
} // namespace tbs