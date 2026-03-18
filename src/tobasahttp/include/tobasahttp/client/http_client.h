#pragma once

#include <tobasa/logger_stdout.h>
#include <tobasa/logger_tobasa.h>
#include "tobasahttp/client/client_traits.h"
#include "tobasahttp/client/client.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

using PlainClientDefault  = Client<ClientTraits<SettingsClient, log::StdoutLogger>>;
using SecureClientDefault = Client<ClientTlsTraits<SettingsClient, log::StdoutLogger>>;

using PlainClient     = Client<ClientTraits<SettingsClient, log::TobasaLogger>>;
using SecureClient    = Client<ClientTlsTraits<SettingsClient, log::TobasaLogger>>;

/** @}*/

} // namespace http
} // namespace tbs