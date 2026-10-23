#pragma once

#include <tobasa/non_copyable.h>
#include <tobasa/path.h>
#include "tobasahttp/server/common.h"
#include "tobasahttp/server/settings_tls.h"
#include "tobasahttp/exception.h"
#include "tobasahttp/util.h"

#ifdef TOBASA_HTTP_USE_HTTP2
   #include "tobasahttp/server/http2_helper.h"
#endif

#include <asio/ssl.hpp>
#include <openssl/objects.h>
#include <openssl/tls1.h>
#include <sstream>
#include "tobasahttp/server/tls_helper.h"


namespace tbs {
namespace http {

/** \addtogroup HTTP
 * @{
 */

/** 
 * \brief Connection starter for https
 * \tparam SettingsType
 * \tparam LoggerType
 */
template<class SettingsType, class LoggerType>
class ConnectionStarterTls : private NonCopyable
{
public:
   using Settings      = SettingsType;
   using Logger        = LoggerType;
   using Socket        = asio::ssl::stream<asio::ip::tcp::socket>;
   using HandshakeType = asio::ssl::stream_base::handshake_type;

private:
   Settings&           _settings;
   Logger&             _logger;
   asio::ssl::context  _tlsContex;
   SniTlsContexts      _sniTlsContexts;
   HandshakeType       _handshakeType;
   InstanceType        _instanceType;

protected:

   void init()
   {
      setupTlsContext();
   }

public:

   ConnectionStarterTls(Settings& settings, Logger& logger)
      : _settings     { settings }
      , _logger       { logger }
      , _tlsContex    { asio::ssl::context::sslv23_server }
      , _instanceType { InstanceType::http_server }
   {
      _handshakeType = asio::ssl::stream_base::server;
#if !defined(NDEBUG)
      _logger.trace("[{}] ConnectionStarterTls initialized", logHttpType());
#endif
      init();
   }

   ~ConnectionStarterTls()
   {
#if !defined(NDEBUG)
      _logger.trace("[{}] ConnectionStarterTls destroyed", logHttpType());
#endif
   }

   /** 
    * \brief Create Connection object, from socket returned by acceptor (we act as server).
    * Also setup connection's onStart handler
    * \tparam ConnectionType
    * \param socket           tcp socket
    * \param requestHandler   request handler
    * \param onCreatedHandler a callback receiving Connection shared_ptr
    *  to be processed further (by connection manager)
    */
   template<class ConnectionType>
   void createConnection(
      asio::ip::tcp::socket socket,
      RequestHandler&       requestHandler,
      std::function<void(std::shared_ptr<ConnectionType>)> onCreatedHandler)
   {
      _logger.info("[{}] Starting HTTPS connection with client {}", logHttpType(), toString(socket.lowest_layer().remote_endpoint()));

      Socket tlsSocket(std::move(socket), _tlsContex);

      // create connection, and setup onStart callback
      auto conn = std::make_shared<ConnectionType>(
         std::move(tlsSocket), _settings, _logger, requestHandler);

      conn->setTls(true);

      // setup connection's onStart functor. onStart will be called from connection's start()
      // which is called from connection manager.
      // after connected TLS socked must initiating handshake
      conn->onStart =
         [handshakeType=this->_handshakeType, 
            wConn = std::weak_ptr<ConnectionType>(conn), &settings=this->_settings,
            logger=&this->_logger, logType=logHttpType(), sniContexts=&this->_sniTlsContexts]
         (Socket& tlsSock, std::function<void(const std::error_code&)> onStartCallback)
         {
            auto initialContext = SSL_get_SSL_CTX(tlsSock.native_handle());
            tlsSock.async_handshake(handshakeType,
               [&tlsSock, onStartCallback=std::move(onStartCallback), 
                  connWeak = std::move(wConn), &settings, logger, logType, initialContext, sniContexts](const std::error_code& error)
               {
                  if (!error)
                  {
                     auto ssl = tlsSock.native_handle();
                     auto activeContext = SSL_get_SSL_CTX(ssl);
                     auto serverName = sniContexts->hostname(ssl);
                     if (!serverName)
                        serverName = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);

                     const unsigned char* alpn = nullptr;
                     unsigned int alpnLength = 0;
                     SSL_get0_alpn_selected(ssl, &alpn, &alpnLength);
                     auto alpnProtocol = alpnLength > 0 ? std::string(reinterpret_cast<const char*>(alpn), alpnLength) : std::string{};
                     logger->trace(
                        "[{}] TLS handshake succeeded: SNI={}, version={}, cipher={}, initial_SSL_CTX={}, active_SSL_CTX={}, ALPN={}",
                        logType,
                        serverName ? serverName : "",
                        SSL_get_version(ssl),
                        SSL_get_cipher(ssl) ? SSL_get_cipher(ssl) : "(none)",
                        static_cast<const void*>(initialContext),
                        static_cast<const void*>(activeContext),
                        alpnProtocol);

                     if (auto c = connWeak.lock()) 
                     {
#ifdef TOBASA_HTTP_USE_HTTP2
                        // Check the selected protocol from ALPN during the TLS handshake
                        // The protocol will be "h2" for HTTP/2, or "http/1.1" for HTTP/1.1
                        if (settings.http2Enabled() && http2::tlsH2Negotiated(tlsSock))
                           c->httpVersion(HttpVersion::two);
                        else
                           c->httpVersion(HttpVersion::one);
#else
                        c->httpVersion(HttpVersion::one);
#endif
                     }
                  }
                  else
                  {
                     auto ssl = tlsSock.native_handle();
                     auto context = SSL_get_SSL_CTX(ssl);
                     auto clientCiphers = SSL_get_client_ciphers(ssl);
                     auto contextCiphers = SSL_CTX_get_ciphers(context);
                     std::ostringstream offered;
                     std::ostringstream configured;

                     for (int i = 0; clientCiphers && i < sk_SSL_CIPHER_num(clientCiphers); ++i)
                     {
                        if (i > 0)
                           offered << ',';
                        offered << SSL_CIPHER_get_name(sk_SSL_CIPHER_value(clientCiphers, i));
                     }

                     for (int i = 0; contextCiphers && i < sk_SSL_CIPHER_num(contextCiphers); ++i)
                     {
                        if (i > 0)
                           configured << ',';
                        configured << SSL_CIPHER_get_name(sk_SSL_CIPHER_value(contextCiphers, i));
                     }

                     auto serverName = sniContexts->hostname(ssl);
                     if (!serverName)
                        serverName = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
                     auto certificate = SSL_CTX_get0_certificate(context);
                     auto privateKey = SSL_CTX_get0_privatekey(context);
                     auto publicKey = certificate ? X509_get0_pubkey(certificate) : nullptr;
                     auto keyType = publicKey ? OBJ_nid2sn(EVP_PKEY_get_base_id(publicKey)) : nullptr;
                     auto keyMatchesCertificate = SSL_CTX_check_private_key(context);
                     auto certificateSuitability = SSL_check_chain(ssl, certificate, privateKey, nullptr);
                     std::ostringstream peerSigalgs;
                     std::ostringstream sharedSigalgs;
                     std::ostringstream sharedGroups;

                     for (int i = 0; ; ++i)
                     {
                        int signNid = 0;
                        int hashNid = 0;
                        int signHashNid = 0;
                        unsigned char rawSign = 0;
                        unsigned char rawHash = 0;
                        if (SSL_get_sigalgs(ssl, i, &signNid, &hashNid, &signHashNid, &rawSign, &rawHash) <= 0)
                           break;
                        if (i > 0)
                           peerSigalgs << ',';
                        peerSigalgs << static_cast<unsigned int>(rawSign) << ':'
                                    << static_cast<unsigned int>(rawHash);
                     }

                     for (int i = 0; ; ++i)
                     {
                        int signNid = 0;
                        int hashNid = 0;
                        int signHashNid = 0;
                        unsigned char rawSign = 0;
                        unsigned char rawHash = 0;
                        if (SSL_get_shared_sigalgs(ssl, i, &signNid, &hashNid, &signHashNid, &rawSign, &rawHash) <= 0)
                           break;
                        if (i > 0)
                           sharedSigalgs << ',';
                        sharedSigalgs << static_cast<unsigned int>(rawSign) << ':'
                                      << static_cast<unsigned int>(rawHash);
                     }

                     for (int i = 0; i < 16; ++i)
                     {
                        int groupNid = SSL_get_shared_group(ssl, i);
                        if (groupNid == 0)
                           break;
                        if (i > 0)
                           sharedGroups << ',';
                        auto groupName = OBJ_nid2sn(groupNid);
                        sharedGroups << (groupName ? groupName : "unknown");
                     }

                     logger->trace(
                        "[{}] TLS handshake failed: {}; SNI={}, version={}, cipher={}, initial_SSL_CTX={}, active_SSL_CTX={}, security_level={}, cert_key={}, key_matches_certificate={}, certificate_suitability=0x{:X}, peer_ciphers=[{}], context_ciphers=[{}], peer_sigalgs=[{}], shared_sigalgs=[{}], shared_groups=[{}]",
                        logType,
                        error.message(),
                        serverName ? serverName : "",
                        SSL_get_version(ssl),
                        SSL_get_cipher(ssl) ? SSL_get_cipher(ssl) : "(none)",
                        static_cast<const void*>(initialContext),
                        static_cast<const void*>(context),
                        SSL_CTX_get_security_level(context),
                        keyType ? keyType : "(none)",
                        keyMatchesCertificate,
                        certificateSuitability,
                        offered.str(),
                        configured.str(),
                        peerSigalgs.str(),
                        sharedSigalgs.str(),
                        sharedGroups.str());
                  }
               
                  onStartCallback(error);
               
               });
         };

      // hand over connection to onCreatedHandler, to be processed further by connection manager
      (onCreatedHandler)(std::move(conn));
   }

private:

   void setupTlsContext()
   {
      try
      {
         _logger.debug("[{}] Configuring TLS context", logHttpType());

         _tlsContex.set_options(
              asio::ssl::context::default_workarounds
            //| asio::ssl::context::no_sslv2
            | asio::ssl::context::single_dh_use);
         
         // privateKeyFile password callback
         if (!_settings.privateKeyPassword().empty()) 
         {
            _tlsContex.set_password_callback([password=_settings.privateKeyPassword()](std::size_t /*max_length*/, asio::ssl::context::password_purpose /*purpose*/) {
               return password;
            });
         } 

         if ( path::exists(_settings.certificateChainFile()) )
            _tlsContex.use_certificate_chain_file( _settings.certificateChainFile() );
         else
         {
            auto certChain = _settings.getDefaultTlsAsset(TlsAsset::cerificate_chain);
            _tlsContex.use_certificate_chain(asio::buffer(certChain.data(), certChain.size()));
         }

         if ( path::exists(_settings.privateKeyFile()) )
            _tlsContex.use_private_key_file(_settings.privateKeyFile(), asio::ssl::context::pem);
         else
         {
            auto privateKey = _settings.getDefaultTlsAsset(TlsAsset::private_key);
            _tlsContex.use_private_key(asio::buffer(privateKey.data(), privateKey.size()), asio::ssl::context::pem);
         }

         if ( path::exists(_settings.tmpDhFile()) )
            _tlsContex.use_tmp_dh_file(_settings.tmpDhFile());
         else
         {
            auto tmpDh = _settings.getDefaultTlsAsset(TlsAsset::tmp_dh);
            if (!tmpDh.empty())
               _tlsContex.use_tmp_dh(asio::buffer(tmpDh.data(), tmpDh.size()));
         }

         auto certChain = _settings.getDefaultTlsAsset(TlsAsset::cerificate_chain);
         auto privateKey = _settings.getDefaultTlsAsset(TlsAsset::private_key);
         auto tmpDh = _settings.getDefaultTlsAsset(TlsAsset::tmp_dh);
         // configure tls for each host-specific context
         _sniTlsContexts.configure(
            _tlsContex,
            _settings.hostCertificates(),
            _settings.tmpDhFile(),
            certChain,
            privateKey,
            tmpDh,
#ifdef TOBASA_HTTP_USE_HTTP2
            _settings.http2Enabled()
#else
            false
#endif
         );

#ifdef TOBASA_HTTP_USE_HTTP2
         // configure tls context for the default context
         if (_settings.http2Enabled())
            http2::configureTlsContext(_tlsContex);
#endif

      }
      catch(const std::exception& ex)
      {
         _logger.debug("[{}] Setting up TLS context has failed: ", logHttpType(), ex.what());
         throw http::Exception( ex.what() );
      }
   }

private:

   std::string logHttpType() const
   {
      return logHttpTypeInfo(_instanceType, _settings.tlsMode());
   }

};

/** @}*/

} // namespace http
} // namespace tbs