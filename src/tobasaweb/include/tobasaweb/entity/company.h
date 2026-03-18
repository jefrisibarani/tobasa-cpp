#pragma once

#include <string>
#include <memory>

namespace tbs {
namespace web {
namespace entity {

/** \ingroup WEB
 * @{
 */

struct Company
{
   int         id;
   std::string name;
   std::string address;
   std::string phone;
   std::string email;
   std::string website;
};

using CompanyPtr = std::shared_ptr<Company>;

/** @}*/

} // namespace entity
} // namespace web
} // namespace tbs