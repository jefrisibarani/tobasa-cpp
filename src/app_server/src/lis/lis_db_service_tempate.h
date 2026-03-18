#pragma once

#include <vector>
#include <tobasa/logger.h>
#include <tobasa/util.h>
#include <tobasa/format.h>
#include <tobasasql/sql_connection.h>
#include <tobasasql/sql_query.h>
#include <tobasasql/sql_log_identifier.h>
#include <tobasasql/sql_service_base.h>
#include "lis_db_service_base.h"

namespace tbs {
namespace lis {
namespace svc {

template <typename SqlDriverType>
class LisServiceTemplate : public LisServiceBase
{
public:
   using SqlResult     = sql::SqlResult<SqlDriverType>;
   using SqlConnection = sql::SqlConnection<SqlDriverType>;
   using SqlQuery      = sql::SqlQuery<SqlDriverType>;
   using Helper        = typename SqlDriverType::HelperImpl;
   using SqlApplyLogId = sql::SqlApplyLogId<SqlDriverType>;

protected:
   SqlConnection& _sqlConn;

public:
   LisServiceTemplate(const LisServiceTemplate &) = delete;
   LisServiceTemplate(LisServiceTemplate &&) = delete;

   LisServiceTemplate() {}
   LisServiceTemplate(SqlConnection& conn) : _sqlConn{ conn }  {}
   virtual ~LisServiceTemplate() {}

   virtual bool saveLisData(const lis::MessagePtr& message, DbStoreResultPtr dbStoreResult=nullptr) override
   {
      return true;
   }   

   virtual std::string dbVersionString() override
   {
      return _sqlConn.databaseName() + " " + _sqlConn.versionString();
   }

};

} // namespace svc
} // namespace lis
} // namespace tbs


