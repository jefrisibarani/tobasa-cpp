#pragma once

#include <string>

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 * \brief PostgreSql Helper.
 */
struct PgsqlHelper
{
   static std::string limitAndOffset(long limit, long offset, const std::string& dbmsName = {});
};


} // namespace sql
} // namespace tbs