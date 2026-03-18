#pragma once

#include <string>

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 * \brief Helper
 * \note Tested only with MS SQL Server
 */
struct OdbcHelper
{
   static std::string limitAndOffset(long limit, long offset, const std::string& dbmsName = {});
};


} // namespace sql
} // namespace tbs