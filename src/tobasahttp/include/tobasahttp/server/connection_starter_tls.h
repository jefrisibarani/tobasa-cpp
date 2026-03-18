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
      _logger.trace("[{}] ConnectionStarterTls initialized", logHttpType());

      init();
   }

   ~ConnectionStarterTls()
   {
      _logger.trace("[{}] ConnectionStarterTls destroyed", logHttpType());
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
            wConn = std::weak_ptr<ConnectionType>(conn), &settings=this->_settings]
         (Socket& tlsSock, std::function<void(const std::error_code&)> onStartCallback)
         {
            tlsSock.async_handshake(handshakeType,
               [&tlsSock, onStartCallback=std::move(onStartCallback), 
                  connWeak = std::move(wConn), &settings](const std::error_code& error)
               {
                  if (!error)
                  {
                     if (auto c = connWeak.lock()) 
                     {
#ifdef TOBASA_HTTP_USE_HTTP2
                        // Check the selected protocol from ALPN during the TLS handshake
                        // The protocol will be "h2" for HTTP/2, or "http/1.1" for HTTP/1.1
                        if (settings.http2Enabled() && http2::tlsH2Negotiated(tlsSock ))
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
                     auto msg = error.message();
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
            _tlsContex.use_tmp_dh(asio::buffer(tmpDh.data(), tmpDh.size()));
         }

         // Register SNI callback with raw OpenSSL
         SSL_CTX_set_tlsext_servername_callback(_tlsContex.native_handle(), sniCallback);
            
         // Prepare extra SSL_CTX objects for other SNI hosts
         // and run http2::configureTlsContext for each context
         if (_settings.hostCertificates().size() > 0) 
         {
            auto certChain  = _settings.getDefaultTlsAsset(TlsAsset::cerificate_chain);
            auto privateKey = _settings.getDefaultTlsAsset(TlsAsset::private_key);
            auto tmpDh      = _settings.getDefaultTlsAsset(TlsAsset::tmp_dh);
            setupCertificateforSNI( 
                 _settings.hostCertificates()
               , _settings.tmpDhFile()
               , certChain
               , privateKey
               , tmpDh 
#ifdef TOBASA_HTTP_USE_HTTP2
               , _settings.http2Enabled()
#endif
            );
         }

#ifdef TOBASA_HTTP_USE_HTTP2
         if (_settings.http2Enabled())
         {
            // Configure TLS context for default context
            http2::configureTlsContext(_tlsContex);
         }
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