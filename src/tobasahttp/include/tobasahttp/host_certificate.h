#pragma once

#include <string>

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

struct HostCertificate
{
   std::string hostname;
   std::string certificateChainFile;
   std::string privateKeyFile;
   std::string password;
};

} // http
} // tbs