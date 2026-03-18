#include <tobasa/path.h>
#include "tobasahttp/server/tls_helper.h"

#ifdef TOBASA_HTTP_USE_HTTP2
   #include "tobasahttp/server/http2_helper.h"
#endif

namespace tbs {
namespace http {


// Map hostnames → SSL_CTX*
std::unordered_map<std::string, std::unique_ptr<asio::ssl::context>> tlsCertMap;

// Lookup function
SSL_CTX* lookupSslCtx(const std::string& hostname) 
{
   auto it = tlsCertMap.find(hostname);
   if (it != tlsCertMap.end()) {
      return it->second->native_handle();
   }
   return nullptr; // fall back to default
}

// SNI callback
int sniCallback(SSL* ssl, int* ad, void* arg) 
{
   const char* servername = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
   if (!servername) 
      return SSL_TLSEXT_ERR_NOACK;

   SSL_CTX* ctx = lookupSslCtx(servername);
   if (ctx) 
   {
      SSL_set_SSL_CTX(ssl, ctx);
      return SSL_TLSEXT_ERR_OK;
   }

   return SSL_TLSEXT_ERR_NOACK;
}


void setupCertificateforSNI(
     const std::vector<HostCertificate>& list
   , const std::string& tmpDhFile
   , nonstd::span<const unsigned char> defaultCertChain
   , nonstd::span<const unsigned char> defaultPrivateKey
   , nonstd::span<const unsigned char> defaulTmpDh
#ifdef TOBASA_HTTP_USE_HTTP2
   , bool http2Enabled
#endif
)
{
   using namespace asio::ssl;
   for (auto& entry: list)
   {
      auto ctx = std::make_unique<context>(context::sslv23_server);
      
      if (path::exists(entry.certificateChainFile))
         ctx->use_certificate_chain_file(entry.certificateChainFile);
      else
         ctx->use_certificate_chain(asio::buffer(defaultCertChain.data(), defaultCertChain.size()) );
      
      if (path::exists(entry.privateKeyFile))
         ctx->use_private_key_file(entry.privateKeyFile, asio::ssl::context::pem);
      else
         ctx->use_private_key( asio::buffer(defaultPrivateKey.data(), defaultPrivateKey.size()), asio::ssl::context::pem );
      
      if (path::exists(tmpDhFile))
         ctx->use_tmp_dh_file(tmpDhFile);
      else{
         ctx->use_tmp_dh( asio::buffer(defaulTmpDh.data(), defaulTmpDh.size()) );
      }

      if (!entry.password.empty()) {
         ctx->set_password_callback([password=entry.password](std::size_t /*max_length*/, asio::ssl::context::password_purpose /*purpose*/) {
               return password; // provide privateKeyFile password
         });
      }

      // SSL_CTX* newCtx = SSL_CTX_new(TLS_server_method());
      // SSL_CTX_use_certificate_chain_file(newCtx, entry.certificateChainFile.c_str());
      // SSL_CTX_use_PrivateKey_file(newCtx, entry.privateKeyFile.c_str(), SSL_FILETYPE_PEM);
      // tlsCertMap[entry.hostname] = newCtx;

#ifdef TOBASA_HTTP_USE_HTTP2
      // Configure HTTP/2 TLS context 
      if (http2Enabled)
         http2::configureTlsContext(*ctx.get() );
#endif

      tlsCertMap[entry.hostname] = std::move(ctx);
   }
}


} // namespace http
} // namespace tbs