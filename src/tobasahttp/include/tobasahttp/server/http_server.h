#pragma once

#include <tobasa/logger_stdout.h>
#include <tobasa/logger_tobasa.h>
#include "tobasahttp/server/traits.h"
#include "tobasahttp/server/server.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

using PlainServerDefault  = Server<Traits<Settings, log::StdoutLogger>>;
using SecureServerDefault = Server<TlsTraits<SettingsTls, log::StdoutLogger>>;

using PlainServer  = Server<Traits<Settings, log::TobasaLogger>>;
using SecureServer = Server<TlsTraits<SettingsTls, log::TobasaLogger>>;

/** @}*/

} // namespace http
} // namespace tbs