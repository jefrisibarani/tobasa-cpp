#include <functional>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <tobasa/logger.h>
#include <tobasa/datetime.h>
#include <tobasa/exception.h>
#include "tobasasql/mysql_variant_helper.h"

namespace tbs {

bool MysqlVariantHelper::isEmpty(const VariantType& variantValue)
{
   try
   {
      if (std::holds_alternative<MysqlTime>(variantValue))
      {
         auto& data = std::get<MysqlTime>(variantValue);
         return data.isEmpty();
      }
      else
      {
         // fallback to check base class method
         return BaseType::isEmpty(variantValue);
      }
   }
   catch (const std::bad_variant_access&)
   {
      throw AppException("MysqlVariantHelper: isEmpty, bad variant access");
   }

   return true;
}

std::string MysqlVariantHelper::toString(const VariantType& variantValue)
{
   std::string strValue;
   try
   {
      if (std::holds_alternative<MysqlTime>(variantValue))
      {
         auto& data = std::get<MysqlTime>(variantValue);
         auto& val = data.myTime;

         auto pad = [](int num) {
            std::stringstream s;
            s << std::setw(2) << std::setfill('0') << num;
            return s.str();
         };

         std::stringstream ss;
         if (val.time_type == MYSQL_TIMESTAMP_TIME)
         {
            if (val.second_part > 0)
               ss << pad(val.hour) << ":" << pad(val.minute) << ":" << pad(val.second) << val.second_part;
            else
               ss << pad(val.hour) << ":" << pad(val.minute) << ":" << pad(val.second);
         }
         else if (val.time_type == MYSQL_TIMESTAMP_DATE) {
            ss << pad(val.year) << "-" << pad(val.month) << "-" << pad(val.day);
         }
         else 
         {
            if (val.second_part > 0)
               ss << pad(val.year) << "-" << pad(val.month) << "-" << pad(val.day) << " " << pad(val.hour) << ":" << pad(val.minute) << ":" << pad(val.second) << val.second_part;
            else
               ss << pad(val.year) << "-" << pad(val.month) << "-" << pad(val.day) << " " << pad(val.hour) << ":" << pad(val.minute) << ":" << pad(val.second);
         }
         
         return ss.str(); 
      }
      else
      {
         // fallback to check base class method
         return BaseType::toString(variantValue);
      }
   }
   catch (const std::bad_variant_access& )
   {
      throw AppException("MysqlVariantHelper: toString, bad variant access");
   }

   return strValue;
}


MysqlTime MysqlVariantHelper::toMysqlTime(const VariantType& variantValue)
{
   try
   {
      if (std::holds_alternative<std::string>(variantValue))
      {
         auto& str = std::get<std::string>(variantValue);
         DateTime dt;

         if (dt.parse(str,"%Y-%m-%d %H:%M:%S") )
         {
            auto ymd = dt.ymd();
            auto hms = dt.hms();
            MysqlTime mt;
            mt.myTime.year   = (int) ymd.year();
            mt.myTime.month  = (unsigned) ymd.month();
            mt.myTime.day    = (unsigned) ymd.day();
            mt.myTime.hour   = (unsigned int) hms.hours().count();
            mt.myTime.minute = (unsigned int) hms.minutes().count();
            mt.myTime.second = (unsigned int) hms.seconds().count();
            mt.myTime.time_type = MYSQL_TIMESTAMP_DATETIME;
            return mt;
         }
         else if (dt.parse(str, "%Y-%m-%d") ) 
         {
            auto ymd = dt.ymd();
            MysqlTime mt;
            mt.myTime.year   = (int) ymd.year();
            mt.myTime.month  = (unsigned) ymd.month();
            mt.myTime.day    = (unsigned) ymd.day();
            mt.myTime.time_type = MYSQL_TIMESTAMP_DATE;
            return mt;
         }
         else if (dt.parseTime(str, "%H:%M:%S") ) // %T
         {
            auto hms = dt.hms();
            MysqlTime mt;
            mt.myTime.hour   = (unsigned int) hms.hours().count();
            mt.myTime.minute = (unsigned int) hms.minutes().count();
            mt.myTime.second = (unsigned int) hms.seconds().count();
            mt.myTime.time_type = MYSQL_TIMESTAMP_TIME;
            return mt;
         }

         return MysqlTime {};
      }
      else
         throw AppException("toMysqlTime source is not a string");
   }
   catch (const std::bad_variant_access& )
   {
      throw AppException("MysqlVariantHelper: toMysqlTime, bad variant access");
   }

   return MysqlTime {};
}

} // namespace tbs