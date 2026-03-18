#pragma once

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#include <comutil.h>
#include <tobasa/variant.h>
#include "tobasasql/common_types.h"

namespace tbs {

/**
 * \ingroup SQL
 * COM Variant type.
 */
using ComVariantType =
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
      , _variant_t
   >;

} // namespace tbs

#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)