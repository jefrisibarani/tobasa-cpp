#pragma once

#include "tobasa/common.h"
#include "tobasa/variant.h"
#include "tobasa/format.h"
#include "tobasa/util_string.h"
#include "tobasa/util_utf.h"
#include "tobasa/exception.h"
#include "tobasa/hextodec.h"

namespace tbs {

/** @addtogroup TBS
 * @{
 */

/** 
 * @brief VariantHelper
 * @tparam VariantTypeImplemented
 */
template <typename VariantTypeImplemented = DefaultVariantType >
class VariantHelper
{
public:

   using VariantType = VariantTypeImplemented;

   /// Check if variant value is empty
   static bool isEmpty(const VariantType& variantValue)
   {
      try
      {
         if (std::holds_alternative<bool>(variantValue))
            return false;
         else if (std::holds_alternative<int8_t>(variantValue))
            return false;
         else if (std::holds_alternative<int16_t>(variantValue))
            return false;
         else if (std::holds_alternative<int32_t>(variantValue))
            return false;
         else if (std::holds_alternative<int64_t>(variantValue))
            return false;
         else if (std::holds_alternative<uint8_t>(variantValue))
            return false;
         else if (std::holds_alternative<uint16_t>(variantValue))
            return false;
         else if (std::holds_alternative<uint32_t>(variantValue))
            return false;
         else if (std::holds_alternative<uint64_t>(variantValue))
            return false;
         else if (std::holds_alternative<float>(variantValue))
            return false;
         else if (std::holds_alternative<double>(variantValue))
            return false;
         else if (std::holds_alternative<std::string>(variantValue))
         {
            auto& val = std::get<std::string>(variantValue);
            return val.empty();
         }
         else if (std::holds_alternative<std::wstring>(variantValue))
         {
            auto& val = std::get<std::wstring>(variantValue);
            return val.empty();
         }
         else if (std::holds_alternative<std::vector<uint8_t>>(variantValue))
         {
            auto& val = std::get<std::vector<uint8_t>>(variantValue);
            return val.empty();
         }
         else if (std::holds_alternative<std::vector<char>>(variantValue))
         {
            auto& val = std::get<std::vector<char>>(variantValue);
            return val.empty();
         }
         else
            throw VariantException("isEmpty, variant type holds unknown alternative", "VariantHelper");
      }
      catch (const std::bad_variant_access&)
      {
         throw VariantException("isEmpty, bad variant access", "VariantHelper");
      }

      return true;
   }

   static bool isSet(const VariantType& variantValue)
   {
      // we use std::monostate as our variant first alternative
      // variantValue.index() == 0 means filled with std::monostate

      if (variantValue.index() > 0)
      {
         // this means variantValue filled with other type
         return true;
      }

      return false;
   }

   static std::string toString(const VariantType& variantValue)
   {
      try
      {
         if (std::holds_alternative<std::monostate>(variantValue))
            return tbs::NULLSTR;
         
         if (std::holds_alternative<bool>(variantValue))
         {
            auto val = std::get<bool>(variantValue);
            return util::boolToStr(val);
         }
         else if (std::holds_alternative<int8_t>(variantValue))
         {
            int8_t val = std::get<int8_t>(variantValue);
            return std::to_string(val);
         }         
         else if (std::holds_alternative<int16_t>(variantValue))
         {
            int16_t val = std::get<int16_t>(variantValue);
            return std::to_string(val);
         }          
         else if (std::holds_alternative<int32_t>(variantValue))
         {
            auto val = std::get<int32_t>(variantValue);
            return std::to_string(val);
         }
         else if (std::holds_alternative<int64_t>(variantValue))
         {
            auto val = std::get<int64_t>(variantValue);
            return std::to_string(val);
         }
         else if (std::holds_alternative<uint8_t>(variantValue))
         {
            uint8_t val = std::get<uint8_t>(variantValue);
            return std::to_string(val);
         }     
         else if (std::holds_alternative<uint16_t>(variantValue))
         {
            uint16_t val = std::get<uint16_t>(variantValue);
            return std::to_string(val);
         }               
         else if (std::holds_alternative<uint32_t>(variantValue))
         {
            auto val = std::get<uint32_t>(variantValue);
            return std::to_string(val);
         }
         else if (std::holds_alternative<uint64_t>(variantValue))
         {
            auto val = std::get<uint64_t>(variantValue);
            return std::to_string(val);
         }
         else if (std::holds_alternative<float>(variantValue))
         {
            auto val = std::get<float>(variantValue);
            return std::to_string(val);
         }
         else if (std::holds_alternative<double>(variantValue))
         {
            auto val = std::get<double>(variantValue);
            return std::to_string(val);
         }         
         else if (std::holds_alternative<std::string>(variantValue))
         {
            return std::get<std::string>(variantValue);
         }
         else if (std::holds_alternative<std::wstring>(variantValue))
         {
            return util::wstring_to_utf8(std::get<std::wstring>(variantValue));
         }
         else if (std::holds_alternative<std::vector<uint8_t>>(variantValue))
         {
            auto& val = std::get<std::vector<uint8_t>>(variantValue);
            return crypt::hexEncode((crypt::byte_t*)val.data(), val.size());
         }
         else if (std::holds_alternative<std::vector<char>>(variantValue))
         {
            auto& val = std::get<std::vector<char>>(variantValue);
            return std::string(val.begin(), val.end());
         }
         else 
            throw VariantException("toString, variant type holds unknown alternative", "VariantHelper");
      }
      catch (const std::bad_variant_access&)
      {
         throw VariantException("toString, bad variant access", "VariantHelper");
      }
      catch (const std::exception& ex)
      {
         throw VariantException(tbsfmt::format("toString, failed: {}", ex.what()), "VariantHelper");
      }

      return "";
   }

   template<typename T>
   static const T& value(const VariantType& variantValue, const std::string& errorMessage={} )
   {
      try
      {
         if (std::holds_alternative<T>(variantValue))
            return std::get<T>(variantValue);
         else
         {
            std::string message = errorMessage.empty() ? std::string("variant type holds unknown alternative") : errorMessage;
            throw VariantException(message, "VariantHelper");
         }
      }
      catch (const std::bad_variant_access& /*ex*/)
      {
         throw VariantException("bad variant access", "VariantHelper");
      }
   }

};

/** @}*/

} // namespace tbs