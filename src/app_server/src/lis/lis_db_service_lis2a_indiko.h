#pragma once

#include "lis_db_service_lis2a.h"

namespace tbs {
namespace lis {
namespace svc {

template < typename SqlDriverType >
class Lis2aServiceIndiko : public Lis2aService<SqlDriverType> 
{
public:
   using SqlResult     = sql::SqlResult<SqlDriverType>;
   using SqlConnection = sql::SqlConnection<SqlDriverType>;
   using SqlQuery      = sql::SqlQuery<SqlDriverType>;
   using Helper        = typename SqlDriverType::HelperImpl;
   using SqlApplyLogId = sql::SqlApplyLogId<SqlDriverType>;

public:

   Lis2aServiceIndiko() {}
   Lis2aServiceIndiko(SqlConnection& conn) 
      : Lis2aService<SqlDriverType>{ conn } {}

   virtual ~Lis2aServiceIndiko() {}

private:
   
   virtual std::string translateSampleDescriptor(const std::string& code) override
   {
      if (code == "1")
         return "Serum";
      else if (code == "2")
         return "Plasma";
      else if (code == "3")
         return "Urine";
      else if (code == "4")
         return "CSF";
      else if (code == "5")
         return "Oral fluid";
      else if (code == "6")
         return "Whole blood";
      else if (code == "7")
         return "Hemol blood";
      else if (code == "8")
         return "Other";
      else
         return code;
   }

   virtual std::string getReferenceRange(const std::string& data, char componentDelimiter) override
   {
      std::string out;

      std::string rangeStr = data;
      std::vector<std::string> rangeArr = util::split(rangeStr, componentDelimiter );
      std::string refrangeLow = rangeArr.size() > 0 ? util::trim(rangeArr[0])  : "";
      std::string refrangeHigh = rangeArr.size() > 1 ? util::trim(rangeArr[1])  : "";

      out   = refrangeLow + " to " + refrangeHigh;
      return out;
   }

};

} // namespace svc
} // namespace lis
} // namespace tbs


