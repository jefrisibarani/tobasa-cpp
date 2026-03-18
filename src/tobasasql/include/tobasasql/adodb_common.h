#pragma once

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#include <functional>
#include "tobasasql/sql_parameter.h"
#include "tobasasql/com_variant.h"

namespace tbs {
namespace sql {

/** \addtogroup SQL
 * @{
 */

/** 
 * ADODB Sql Parameter implementation.
 * AdoParameter accepts _variant_t as it value
 */
using AdoParameter               = Parameter<ComVariantType>;

/// AdoParameter shared pointer.
using AdoParameterPtr            = std::shared_ptr<AdoParameter>;

/// AdoParameter collection.
using AdoParameterCollection     = std::vector<AdoParameterPtr>;

/// AdoParameter collection shared pointer.
using AdoParameterCollectionPtr  = std::shared_ptr<AdoParameterCollection>;

/** @}*/

} // namespace sql
} // namespace tbs

#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)