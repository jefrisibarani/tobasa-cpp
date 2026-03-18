#pragma once

#include <cstdint>
#include <functional>
#include <variant>

namespace tbs {

/** \addtogroup TBS
 * @{
 */

template<typename... Types>
using Variant = std::variant<Types...>;

using DefaultVariantType =
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
   >;

/** @}*/

} // namespace tbs