#pragma once

#include <string>
#include <memory>
#include <tobasa/datetime.h>

namespace tbs {
namespace web {
namespace entity {

/** \ingroup WEB
 * @{
 */

/** 
 * AuthLog
 */
struct AuthLog
{
   long        id;
   DateTime    logonTime;
   long        usrId;
   std::string usrName;
   std::string textNote;
   std::string srcIp;
   std::string srcHost;
   std::string srcMac;
   std::string authType;
   long        siteId;
};

using AuthLogPtr = std::shared_ptr<AuthLog>;

/** @}*/

} // namespace entity
} // namespace web
} // namespace tbs