#pragma once

#include <tobasa/non_copyable.h>
#include <tobasa/path.h>
#include "tobasahttp/client/response.h"
#include "tobasahttp/client/settings.h"
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
class ClientConnectionStarterTls : private NonCopyable
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

   ClientConnectionStarterTls(Settings& settings, Logger& logger)
      : _settings     { settings }
      , _logger       { logger }
      , _tlsContex    { asio::ssl::context::sslv23_client }
      , _instanceType { InstanceType::http_client }
   {
      _handshakeType = asio::ssl::stream_base::client;
      _settings.tlsMode(true);
      _logger.trace("[{}] ClientConnectionStarterTls initialized", logHttpType());

      init();
   }

   ~ClientConnectionStarterTls()
   {
      _logger.trace("[{}] ClientConnectionStarterTls destroyed", logHttpType());
   }


   /**
    * \brief Create https client session
    * \param socket
    * \param responseHandler
    * \param onCreateHandler
    */
   template<class ConnectionType>
   void createClientSession(
      asio::ip::tcp::socket socket,
      ClientResponseHandler responseHandler,
      std::function<void(std::shared_ptr<ConnectionType>)> onCreatedHandler)
   {
      _logger.debug("[{}] Starting HTTPS client connection with server", logHttpType());

      Socket tlsSocket(std::move(socket), _tlsContex);

      if (_settings.verifyPeer() && _settings.caVerificationFile().size() > 0)
      {
         tlsSocket.set_verify_callback(
            std::bind(&ClientConnectionStarterTls::onVerifyTls, this, std::placeholders::_1, std::placeholders::_2));
      }

      // create connection, and setup onStart callback
      auto conn = std::make_shared<ConnectionType>(
         std::move(tlsSocket), _settings, _logger, responseHandler);

      conn->setTls(true);
      //conn->instanceType(_instanceType);

      // setup connection's onStart functor. onStart will be called from connection's start()
      // which is called from connection manager.
      // onStartCallback will be defined inside connection's start() upon calling onStart()
      // after connected TLS socked must initiating handshake
      conn->onStart =
         [handshakeType=this->_handshakeType, settings=this->_settings]
         (Socket& tlsSock, std::function<void(const std::error_code&)> onStartCallback)
         {
            // Note: always got stream truncated on server with virtual hosts
            // https://en.wikipedia.org/wiki/Server_Name_Indication
            // https://stackoverflow.com/a/71081152
            SSL_set_tlsext_host_name(tlsSock.native_handle(), settings.address().c_str());

            // Set onStartCallback as completion_token
            tlsSock.async_handshake(handshakeType, onStartCallback);
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
         if (!_settings.privateKeyPassword().empty()) {
            _tlsContex.set_password_callback([password=_settings.privateKeyPassword()](std::size_t /*max_length*/, asio::ssl::context::password_purpose /*purpose*/) {
               return password;
            });
         } 

         if (  _settings.verifyPeer() && _settings.caVerificationFile().size() > 0) 
         {
            _tlsContex.set_default_verify_paths();
            // Load the certificates for one or more trusted certification authorities from a file.
            _tlsContex.load_verify_file(_settings.caVerificationFile());
            _tlsContex.set_verify_mode(
                  asio::ssl::verify_peer
               | asio::ssl::verify_fail_if_no_peer_cert
               | asio::ssl::verify_client_once);
         }
         else {
            _tlsContex.set_verify_mode(asio::ssl::verify_none );
         }

      // #ifdef TOBASA_HTTP_USE_HTTP2
      //    if (_settings.http2Enabled())
      //    {
      //       // Advertise ALPN protocols
      //       unsigned int len;
      //       auto alpnProtos = http2::getAlpnProtos(len);
      //       auto rval = SSL_CTX_set_alpn_protos(_tlsContex.native_handle(), alpnProtos, len );
      //       if ( rval != 0 ) 
      //       {
      //          _logger.debug("[{}] Setting up TLS context has failed: ", logHttpType(), "can not set alpn protos" );
      //          throw std::runtime_error( "failed to set ALPN protocols" );
      //       }
      //    }
      //  #endif

      }
      catch(const std::exception& ex)
      {
         _logger.debug("[{}] Setting up TLS context has failed: ", logHttpType(), ex.what());
         throw http::Exception( ex.what() );
      }
   }

   bool onVerifyTls(bool preverified, asio::ssl::verify_context& ctx)
   {
      // TODO_JEFRI: Complete this!

      // The verify callback can be used to check whether the certificate that is
      // being presented is valid for the peer. For example, RFC 2818 describes
      // the steps involved in doing this for HTTPS. Consult the OpenSSL
      // documentation for more details. Note that the callback is called once
      // for each certificate in the certificate chain, starting from the root
      // certificate authority.

      // In this example we will simply print the certificate's subject name.
      char subjectName[256];
      X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
      X509_NAME_oneline(X509_get_subject_name(cert), subjectName, 256);

      if (!preverified)
         _logger.debug("[{}] SSL verification failed, subject name: {}", logHttpType(), subjectName);
      else
         _logger.debug("[{}] SSL verified successfully, subject name: {}", logHttpType(), subjectName);

      return preverified;
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