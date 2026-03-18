#pragma once

#include <tobasa/variant_helper.h>
#include "tobasasql/mysql_variant.h"

namespace tbs {

/** 
 * \ingroup SQL
 * \brief MySql Variant helper
 */
class MysqlVariantHelper : public VariantHelper<MysqlVariantType>
{
public:

   using VariantType   = MysqlVariantType;
   using BaseType      = VariantHelper<MysqlVariantType>;

   /// Check if variant value is empty.
   /// TODO_JEFRI: what is empty?
   static bool isEmpty(const VariantType& variantValue);

   static std::string toString(const VariantType& variantValue);

   static MysqlTime toMysqlTime(const VariantType& variantValue);
};

} // namespace tbs
