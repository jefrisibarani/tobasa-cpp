#pragma once

#include <string>
#include <memory>
#include "tobasa/datetime.h"

namespace tbs {
namespace web {
namespace entity {

/** \ingroup WEB
 * @{
 */

struct UserResetPassword
{
   long        id;
   long        userId;
   DateTime    requestTime;
   long        expiredTime;
   std::string resetCode;
   bool        success;
   DateTime    successTime;
};

using UserResetPasswordPtr = std::shared_ptr<UserResetPassword>;

/** @}*/

} // namespace entity
} // namespace web
} // namespace tbs