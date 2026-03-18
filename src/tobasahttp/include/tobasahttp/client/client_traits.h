#pragma once

#include <asio.hpp>
#include <asio/ssl.hpp>
#include "tobasahttp/nologger.h"
#include "tobasahttp/settings.h"
#include "tobasahttp/client/settings.h"
#include "tobasahttp/client/connection_starter.h"
#include "tobasahttp/client/connection_starter_tls.h"

namespace tbs {
namespace http {

/** \defgroup HTTP Http Server library.
 * @{
 */

/**
 * \brief Client traits
 * \tparam SettingsType
 * \tparam LoggerType
 * \tparam SocketType
 */
template <
   typename SettingsType,
   typename LoggerType,
   typename SocketType = asio::ip::tcp::socket>
struct ClientTraits
{
   using Settings          = SettingsType;
   using Logger            = LoggerType;
   using Socket            = SocketType;
   using ConnectionStarter = http::ClientConnectionStarter<Settings, LoggerType>;
};
using DefaultClientTraits = ClientTraits<SettingsClient, NoLogger>;


/** 
 * \ingroup HTTP.
 * \brief Client TLS Traits.
 * \tparam SettingsType
 * \tparam LoggerType
 */
template<typename SettingsType, typename LoggerType>
struct ClientTlsTraits
   : public ClientTraits<SettingsType, LoggerType>
{
   using Socket            = asio::ssl::stream<asio::ip::tcp::socket>;
   using ConnectionStarter = http::ClientConnectionStarterTls<SettingsType, LoggerType>;
};
using DefaultClientTlsTraits = ClientTlsTraits<SettingsClient, NoLogger>;

/** @}*/

} // namespace http
} // namespace tbs