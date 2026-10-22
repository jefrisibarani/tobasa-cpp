#pragma once

#include <string>
#include <iostream>
#include <new>
#include <memory>
#include <unordered_map>
#include <vector>
#include <tobasa/path.h>
#include "tobasahttp/server/settings_tls.h"
#include "tobasahttp/exp_imp.h"
#ifdef TOBASA_HTTP_USE_HTTP2
#include "tobasahttp/server/http2_helper.h"
#endif

namespace tbs {
namespace http {

/// Lookup function
TBS_HTTP_API SSL_CTX* lookupSslCtx(const std::string& hostname);

/// Return the SNI hostname saved for this SSL connection, if available.
TBS_HTTP_API const char* getSniHostname(const SSL* ssl);

/// SNI callback
TBS_HTTP_API int sniCallback(SSL* ssl, int* ad, void* arg);

TBS_HTTP_API void setupCertificateforSNI(
     const std::vector<HostCertificate>& list
   , const std::string& tmpDhFile
   , nonstd::span<const unsigned char> defaultCertChain
   , nonstd::span<const unsigned char> defaultPrivateKey
   , nonstd::span<const unsigned char> defaulTmpDh
#ifdef TOBASA_HTTP_USE_HTTP2
   , bool http2Enabled
#endif
);

class SniTlsContexts final
{
   std::unordered_map<std::string, std::unique_ptr<asio::ssl::context>> _contexts;

   static void freeHostname(void*, void* data, CRYPTO_EX_DATA*, int, long, void*)
   {
      delete static_cast<std::string*>(data);
   }

   static int hostnameIndex()
   {
      static int index = SSL_get_ex_new_index(0, nullptr, nullptr, nullptr, freeHostname);
      return index;
   }

   static void saveHostname(SSL* ssl, const char* hostname)
   {
      auto index = hostnameIndex();
      if (index < 0)
         return;

      try
      {
         auto value = new std::string(hostname);
         auto previous = static_cast<std::string*>(SSL_get_ex_data(ssl, index));
         if (SSL_set_ex_data(ssl, index, value) == 1)
            delete previous;
         else
            delete value;
      }
      catch (...)
      {
      }
   }

   static int selectContext(SSL* ssl, int*, void* arg)
   {
      auto self = static_cast<SniTlsContexts*>(arg);
      auto hostname = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
      if (!self || !hostname)
         return SSL_TLSEXT_ERR_NOACK;

      saveHostname(ssl, hostname);
      auto found = self->_contexts.find(hostname);
      if (found == self->_contexts.end())
         return SSL_TLSEXT_ERR_NOACK;

      SSL_set_SSL_CTX(ssl, found->second->native_handle());
      return SSL_TLSEXT_ERR_OK;
   }

public:
   const char* hostname(const SSL* ssl) const
   {
      auto index = hostnameIndex();
      if (!ssl || index < 0)
         return nullptr;

      auto value = static_cast<const std::string*>(SSL_get_ex_data(ssl, index));
      return value ? value->c_str() : nullptr;
   }

   void configure(
      asio::ssl::context& defaultContext,
      const std::vector<HostCertificate>& hosts,
      const std::string& tmpDhFile,
      nonstd::span<const unsigned char> defaultCertChain,
      nonstd::span<const unsigned char> defaultPrivateKey,
      nonstd::span<const unsigned char> defaultTmpDh,
      bool http2Enabled)
   {
      auto defaultHandle = defaultContext.native_handle();
      SSL_CTX_set_tlsext_servername_callback(defaultHandle, selectContext);
      SSL_CTX_set_tlsext_servername_arg(defaultHandle, this);

      for (const auto& entry : hosts)
      {
         auto context = std::make_unique<asio::ssl::context>(asio::ssl::context::sslv23_server);
         if (path::exists(entry.certificateChainFile))
            context->use_certificate_chain_file(entry.certificateChainFile);
         else
            context->use_certificate_chain(asio::buffer(defaultCertChain.data(), defaultCertChain.size()));

         if (path::exists(entry.privateKeyFile))
            context->use_private_key_file(entry.privateKeyFile, asio::ssl::context::pem);
         else
            context->use_private_key(asio::buffer(defaultPrivateKey.data(), defaultPrivateKey.size()), asio::ssl::context::pem);

         if (path::exists(tmpDhFile))
            context->use_tmp_dh_file(tmpDhFile);
         else if (!defaultTmpDh.empty())
            context->use_tmp_dh(asio::buffer(defaultTmpDh.data(), defaultTmpDh.size()));

         if (!entry.password.empty())
         {
            context->set_password_callback([password=entry.password](std::size_t, asio::ssl::context::password_purpose) {
               return password;
            });
         }

#ifdef TOBASA_HTTP_USE_HTTP2
         if (http2Enabled)
            http2::configureTlsContextInModule(*context);
#endif

         _contexts[entry.hostname] = std::move(context);
      }
   }
};

} // namespace http
} // namespace tbs