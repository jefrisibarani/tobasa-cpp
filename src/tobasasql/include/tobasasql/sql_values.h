#pragma once

#include <string>

namespace tbs {
namespace sql {

/** \addtogroup SQL
 * @{
 */



struct SqlDecimal
{
   std::string value;
   uint8_t precision = -1;
   uint8_t scale     = -1;
};


/** @}*/

} // namespace sql
} // namespace tbs