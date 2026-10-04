#pragma once

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#include <cstdint>

namespace tbs {
namespace sql {

/** \addtogroup SQL
 * @{
 */

struct AdoDbTime2
{
   uint16_t hour;
   uint16_t minute;
   uint16_t second;
   uint32_t fraction;
};

} // namespace sql
} // namespace tbs

#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)