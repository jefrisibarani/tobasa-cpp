#pragma once

#include <string>

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 * \brief Mysql Helper
 * \note Experimental
 */
struct MysqlHelper
{
   static std::string limitAndOffset(long limit, long offset, const std::string& dbmsName = {});
};


} // namespace sql
} // namespace tbs