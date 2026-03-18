#pragma once

#include <tobasasql/sql_query_option.h>
#include "tobasaweb/router.h"

namespace tbs {
namespace util {


sql::SqlQueryOption getSqlQueryOption(
   const web::RouteArgument& arg,
   std::string& outStartDate,
   std::string& outEndDate,
   std::string& outErrorMessage);


/** 
 * Create Secure Hash.
 * \param message
 * \param messageHashOut
 * \param saltHashOut reference to hash result
 */
void createSecureHash(const std::string& message,std::string& messageHashOut, std::string& saltHashOut);


/** 
 * Verify Secure Hash.
 * \param message
 * \param storedMessageHash
 * \param storedSaltHash
 * \return True if hash ok
 */
bool verifySecureHash(const std::string& message, const std::string& storedMessageHash, const std::string& storedSaltHash);


} // namespace util
} // namespace tbs