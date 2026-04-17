#pragma once

#include <asio/ip/tcp.hpp>
#include <string>
#include <cstdint>
#include <algorithm>
#include "tobasahttp/settings.h"

namespace tbs {
namespace http {


/** \addtogroup HTTP
 * @{
 */

/** 
 * HTTP Client settings.
 */
class SettingsClient final
   :public SettingsBase<SettingsClient>
{
   using BaseType = SettingsBase<SettingsClient>;
public:
   using BaseType::BaseType;

private:
   int32_t        _connectionPoolSize     {1};

   // mTLS client
  // ------------------------------------------------
   std::string    _certificateFile        {}; // client.crt
   std::string    _privateKeyFile         {}; // client.key
   std::string    _privateKeyPassword     {};

   /// Certificate Authorities certificates in PEM format 
   std::string    _caVerificationFile     {}; // ca.pem
   bool           _verifyPeer             { false };
   /// TLS in server mode
   bool           _serverMode             { false };

public:
   SettingsClient(
      std::string address = "localhost",
      uint16_t port = 8084,
      asio::ip::tcp protocol = asio::ip::tcp::v4() )
      : BaseType(address, port, protocol)
   {}

   // ------------------------------------------------
   // default 1, max 1000
   SettingsClient& connectionPoolSize(int32_t val) &
   {
      _connectionPoolSize = self().clampValue(val, (int32_t)1, (int32_t)1000, (int32_t)1);
      return self();
   }
   SettingsClient&& connectionPoolSize(int32_t val) &&
   {
      return std::move(self().connectionPoolSize(val));
   }
   [[nodiscard]] int32_t connectionPoolSize() const { return _connectionPoolSize; }

   // ------------------------------------------------
   SettingsClient& caVerificationFile(std::string val) &
   {
      _caVerificationFile = std::move(val);
      return self();
   }
   SettingsClient&& caVerificationFile(std::string val) &&
   {
      return std::move(self().caVerificationFile(val));
   }
   std::string caVerificationFile() const { return _caVerificationFile; }

   // ------------------------------------------------
   SettingsClient& verifyPeer(bool val) &
   {
      _verifyPeer = val;
      return self();
   }
   SettingsClient&& verifyPeer(bool val) &&
   {
      return std::move(self().verifyPeer(val));
   }
   bool verifyPeer() const { return _verifyPeer; }


   // ------------------------------------------------
   SettingsClient& certificateFile(std::string val) &
   {
      _certificateFile = std::move(val);
      return self();
   }
   SettingsClient&& certificateFile(std::string p) &&
   {
      return std::move(self().certificateFile(p));
   }
   [[nodiscard]]
   std::string certificateFile() const { return _certificateFile; }

   // ------------------------------------------------
   SettingsClient& privateKeyFile(std::string val) &
   {
      _privateKeyFile = std::move(val);
      return self();
   }
   SettingsClient&& privateKeyFile(std::string val) &&
   {
      return std::move(self().privateKeyFile(std::move(val)));
   }
   [[nodiscard]]
   std::string privateKeyFile() const { return _privateKeyFile; }

   // ------------------------------------------------
   SettingsClient& privateKeyPassword(std::string val) &
   {
      _privateKeyPassword = std::move(val);
      return self();
   }
   SettingsClient&& privateKeyPassword(std::string val) &&
   {
      return std::move(self().privateKeyPassword(val));
   }
   std::string privateKeyPassword() const { return _privateKeyPassword; }

};


/** @}*/

} // namespace http
} // namespace tbs