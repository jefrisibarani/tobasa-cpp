#pragma once

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#include <tobasa/variant_helper.h>
#include "tobasasql/com_variant.h"

namespace tbs {

/** 
 * \ingroup SQL
 * \brief COM Variant helper
 * \sa https://docs.microsoft.com/en-us/windows/win32/api/oaidl/ns-oaidl-variant
 * \sa https://docs.microsoft.com/en-us/windows/win32/api/wtypes/ne-wtypes-varenum
 * \sa https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-oaut/3fe7db9f-5803-4dc4-9d14-5425d3f5461f
 */
class ComVariantHelper : public VariantHelper<ComVariantType>
{
public:
   using VariantType   = ComVariantType;
   using BaseType      = VariantHelper<ComVariantType>;
   using NativeVariant = _variant_t;

   /// Check if variant value is empty.
   /// TODO: what is empty??
   static bool isEmpty(const VariantType& variantValue);

   static std::string toString(const VariantType& variantValue);

   /** 
    * Convert _variant_t to std::string.
    * \param[in]  vSource  _variant_t value
    * \param[out] outStr   refrecence to std::string value
    */
   static void nativeVariantToString(const _variant_t& vSource, std::string& outStr);

   /// Convert AdovariantType to _variant_t.
   static NativeVariant toNativeVariant(const VariantType& variantVal);

   static VariantType fromNativeVariant(const _variant_t& vSource);
};

} // namespace tbs

#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)