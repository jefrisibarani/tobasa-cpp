#pragma once

#include "lis_db_service_lis2a.h"

namespace tbs {
namespace lis {
namespace svc {

template < typename SqlDriverType >
class Lis2aServiceDxH500 : public Lis2aService<SqlDriverType> 
{
public:
   using SqlResult     = sql::SqlResult<SqlDriverType>;
   using SqlConnection = sql::SqlConnection<SqlDriverType>;
   using SqlQuery      = sql::SqlQuery<SqlDriverType>;
   using Helper        = typename SqlDriverType::HelperImpl;
   using SqlApplyLogId = sql::SqlApplyLogId<SqlDriverType>;
   
public:
   Lis2aServiceDxH500() {}
   Lis2aServiceDxH500(SqlConnection& conn) 
      : Lis2aService<SqlDriverType>{ conn }  {}

   virtual ~Lis2aServiceDxH500() {}

private:

   virtual std::string translateSampleDescriptor(const std::string& code) override
   {
      if (code == "WB")
         return "Whole Blood";
      else if (code == "PD")
         return "Predilute";
      else
         return code;
   }

};

} // namespace svc
} // namespace lis
} // namespace tbs


