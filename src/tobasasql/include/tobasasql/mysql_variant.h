#pragma once

#include <cstring>
#include <tobasa/variant.h>
#include <mysql/mysql.h>

namespace tbs {


class MysqlTime 
{
public:
   MYSQL_TIME myTime;

   MysqlTime()
   {
      memset(&myTime, 0, sizeof(MYSQL_TIME)); 
   }

   bool operator==(const MysqlTime& other) const 
   {
      return myTime.year == other.myTime.year &&
             myTime.month == other.myTime.month &&
             myTime.day == other.myTime.day &&
             myTime.hour == other.myTime.hour &&
             myTime.minute == other.myTime.minute &&
             myTime.second == other.myTime.second &&
             myTime.second_part == other.myTime.second_part &&
             myTime.neg == other.myTime.neg &&
             myTime.time_type == other.myTime.time_type;
   }

   bool operator!=(const MysqlTime& other) const 
   {
      return !(*this == other);
   }

   bool isEmpty() const 
   {
      return myTime.year == 0 &&
             myTime.month == 0 &&
             myTime.day == 0 &&
             myTime.hour == 0 &&
             myTime.minute == 0 &&
             myTime.second == 0 &&
             myTime.second_part == 0 &&
             myTime.neg == 0 &&
             myTime.time_type == 0;
   }
};

/**
 * \ingroup SQL
 * MySQL Variant type.
 */
using MysqlVariantType =
   Variant<
        std::monostate
      , bool
      , std::int8_t          
      , std::int16_t         
      , std::int32_t         
      , std::int64_t         
      , std::uint8_t         
      , std::uint16_t        
      , std::uint32_t        
      , std::uint64_t        
      , float                
      , double               
      , std::string          
      , std::wstring         
      , std::vector<uint8_t> 
      , std::vector<char>    
      , MysqlTime
   >;

} // namespace tbs