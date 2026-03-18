#pragma once

#include <asio/ssl.hpp>
#include <tobasa/span.h>
#include "tobasahttp/host_certificate.h"
#include "tobasahttp/server/settings.h"

namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */


enum class TlsAsset
{
   cerificate_chain,
   private_key,
   tmp_dh,
   verify_file
};

using DefaultTlsAssetCallback = std::function<nonstd::span<const unsigned char>(TlsAsset)>;


/** 
 * HTTPS Server settings.
 */
class SettingsTls final
   :	public SettingsServerCommon<SettingsTls>
{
   using BaseType = SettingsServerCommon<SettingsTls>;

private:
   std::string _certificateChainFile {"localhost.crt"};
   std::string _privateKeyFile       {"localhost.key"};
   std::string _privateKeyPassword   {};
   std::string _tmpDhFile            {"dh2048.pem"};

   /// TLS in server mode
   bool        _serverMode           { true };

   std::vector<HostCertificate> _hosCertificates;

   DefaultTlsAssetCallback _defaultTlsAssetCallback;

public:
   using BaseType::BaseType;

   SettingsTls(
      std::string   address  = "localhost",
      std::uint16_t port     = 8085,
      asio::ip::tcp protocol = asio::ip::tcp::v4() )
      : BaseType(address, port, protocol)
   {
      _tlsMode = true;
   }

   // ------------------------------------------------
   SettingsTls& certificateChainFile(std::string val) &
   {
      _certificateChainFile = std::move(val);
      return self();
   }
   SettingsTls&& certificateChainFile(std::string p) &&
   {
      return std::move(self().certificateChainFile(p));
   }
   [[nodiscard]]
   std::string certificateChainFile() const { return _certificateChainFile; }

   // ------------------------------------------------
   SettingsTls& privateKeyFile(std::string val) &
   {
      _privateKeyFile = std::move(val);
      return self();
   }
   SettingsTls&& privateKeyFile(std::string val) &&
   {
      return std::move(self().privateKeyFile(std::move(val)));
   }
   [[nodiscard]]
   std::string privateKeyFile() const { return _privateKeyFile; }

   // ------------------------------------------------
   SettingsTls& privateKeyPassword(std::string val) &
   {
      _privateKeyPassword = std::move(val);
      return self();
   }
   SettingsTls&& privateKeyPassword(std::string val) &&
   {
      return std::move( self().privateKeyPassword( std::move( val ) ) );
   }
   [[nodiscard]]
   std::string privateKeyPassword() const { return _privateKeyPassword; }

   // // ------------------------------------------------
   // SettingsTls& caVerificationFile(std::string val) &
   // {
   //    _caVerificationFile = std::move(val);
   //    return self();
   // }
   // SettingsTls&& caVerificationFile(std::string val) &&
   // {
   //    return std::move( self().caVerificationFile( std::move( val ) ) );
   // }
   // [[nodiscard]]
   // std::string caVerificationFile() const { return _caVerificationFile; }

   // ------------------------------------------------
   SettingsTls& tmpDhFile(std::string val) &
   {
      _tmpDhFile = std::move(val);
      return self();
   }
   SettingsTls&& tmpDhFile(std::string val) &&
   {
      return std::move( self().tmpDhFile( std::move( val ) ) );
   }
   [[nodiscard]]
   std::string tmpDhFile() const { return _tmpDhFile; }

   // ------------------------------------------------
   SettingsTls& hostCertificates(std::vector<HostCertificate>val) &
   {
      _hosCertificates = std::move(val);
      return self();
   }
   SettingsTls&& hostCertificates(std::vector<HostCertificate>val) &&
   {
      return std::move( self().hostCertificates(std::move(val)) );
   }
   [[nodiscard]]
   std::vector<HostCertificate>& hostCertificates() { return _hosCertificates; }

   // // ------------------------------------------------
   // SettingsTls& verifyPeer(bool val) &
   // {
   //    _verifyPeer = val;
   //    return self();
   // }
   // SettingsTls&& verifyPeer(bool val) &&
   // {
   //    return std::move( self().verifyPeer( val ) );
   // }
   // [[nodiscard]]
   // bool verifyPeer() const {
   //     return _verifyPeer; }

   // ------------------------------------------------
   SettingsTls& serverMode(bool val) &
   {
      _serverMode = val;
      return self();
   }
   SettingsTls&& serverMode(bool val) &&
   {
      return std::move( self().serverMode( val ) );
   }
   [[nodiscard]] bool serverMode() const { return _serverMode; }

   // ------------------------------------------------
   SettingsTls& defaultTlsAssetCallback(DefaultTlsAssetCallback cb) &
   {
      _defaultTlsAssetCallback = std::move(cb);
      return self();
   }
   SettingsTls&& defaultTlsAssetCallback( DefaultTlsAssetCallback cb ) &&
   {
      return std::move( self().defaultTlsAssetCallback( cb ) );
   }
   [[nodiscard]] nonstd::span<const unsigned char> getDefaultTlsAsset(TlsAsset asset) const
   {
      if (_defaultTlsAssetCallback)
         return _defaultTlsAssetCallback(asset);
      else
         return {};
   }

};

/** @}*/

} // namespace http
} // namespace tbs