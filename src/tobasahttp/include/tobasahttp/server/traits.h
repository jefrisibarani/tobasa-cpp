#pragma once

#include <asio.hpp>
#include "tobasahttp/nologger.h"
#include "tobasahttp/server/settings.h"
#include "tobasahttp/server/settings_tls.h"
#include "tobasahttp/server/connection_starter.h"
#include "tobasahttp/server/connection_starter_tls.h"

namespace tbs {
namespace http {

/** \defgroup HTTP Http Server library.
 * @{
 */

/**
 * \brief Server traits
 * \tparam SettingsType
 * \tparam LoggerType
 * \tparam SocketType
 */
template <
   typename SettingsType,
   typename LoggerType,
   typename SocketType = asio::ip::tcp::socket>
struct Traits
{
   using Settings          = SettingsType;
   using Logger            = LoggerType;
   using Socket            = SocketType;
   using ConnectionStarter = http::ConnectionStarter<Settings, LoggerType>;
};


/**
 * \brief Server TLS traits
 * \tparam SettingsType
 * \tparam LoggerType
 * \tparam SocketType
 */
template<typename SettingsType, typename LoggerType>
struct TlsTraits
   : public Traits<SettingsTls, LoggerType>
{
   using Socket            = asio::ssl::stream<asio::ip::tcp::socket>;
   using ConnectionStarter = ConnectionStarterTls<SettingsTls, LoggerType>;
};

/** @}*/

} // namespace http
} // namespace tbs