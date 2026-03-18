#ifdef TOBASA_HTTP_USE_HTTP2

#include "tobasa/util_string.h"
#include "tobasahttp/server/http2_helper.h"

namespace tbs {
namespace http2 {


/* Conditional logic w/ lookup tables to check if id is one of the
   the block listed cipher suites for HTTP/2 described in RFC 7540.
   https://github.com/jay/http2_blacklisted_ciphers
*/
#define IS_CIPHER_BANNED_METHOD2(id)                                           \
  ((0x0000 <= id && id <= 0x00FF &&                                            \
    "\xFF\xFF\xFF\xCF\xFF\xFF\xFF\xFF\x7F\x00\x00\x00\x80\x3F\x00\x00"         \
    "\xF0\xFF\xFF\x3F\xF3\xF3\xFF\xFF\x3F\x00\x00\x00\x00\x00\x00\x80"         \
        [(id & 0xFF) / 8] &                                                    \
      (1 << (id % 8))) ||                                                      \
   (0xC000 <= id && id <= 0xC0FF &&                                            \
    "\xFE\xFF\xFF\xFF\xFF\x67\xFE\xFF\xFF\xFF\x33\xCF\xFC\xCF\xFF\xCF"         \
    "\x3C\xF3\xFC\x3F\x33\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"         \
        [(id & 0xFF) / 8] &                                                    \
      (1 << (id % 8))))

bool checkHttp2TlsVersion(SSL *ssl) 
{
  auto tls_ver = SSL_version(ssl);
  return tls_ver >= TLS1_2_VERSION;
}

bool checkHttp2CipherBlockList(SSL *ssl)
{
   int id = SSL_CIPHER_get_id(SSL_get_current_cipher(ssl)) & 0xFFFFFF;
   return IS_CIPHER_BANNED_METHOD2(id);
}

bool checkHttp2Requirement(SSL *ssl) 
{
  return checkHttp2TlsVersion(ssl) && !checkHttp2CipherBlockList(ssl);
}



#ifndef OPENSSL_NO_NEXTPROTONEG
namespace {
std::vector<unsigned char> &getAlpnToken() 
{
   static auto alpn_token = getDefaultAlpn();
   return alpn_token;
}
} // namespace
#endif // !OPENSSL_NO_NEXTPROTONEG

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
namespace {
int alpnSelectProtoCb(SSL *ssl, const unsigned char **out,
                      unsigned char *outlen, const unsigned char *in,
                      unsigned int inlen, void *arg) 
{
   if (!selectH2(out, outlen, in, inlen)) {
      return SSL_TLSEXT_ERR_NOACK;
   }

   return SSL_TLSEXT_ERR_OK;
}
} // namespace
#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L

namespace {
bool selectProto(const unsigned char **out, unsigned char *outlen,
                  const unsigned char *in, unsigned int inlen,
                  const StringRef &key) 
{
   for (auto p = in, end = in + inlen; p + key.size() <= end; p += *p + 1) 
   {
      if (std::equal(std::begin(key), std::end(key), p)) 
      {
         *out = p + 1;
         *outlen = *p;
         return true;
      }
  }
  return false;
}
} // namespace


bool checkH2IsSelected(const StringRef &proto) 
{
  return util::streq(NGHTTP2_H2, proto) || 
         util::streq(NGHTTP2_H2_16, proto) ||
         util::streq(NGHTTP2_H2_14, proto);
}

bool selectH2(const unsigned char **out, unsigned char *outlen,
               const unsigned char *in, unsigned int inlen) 
{
   return selectProto(out, outlen, in, inlen, NGHTTP2_H2_ALPN) ||
          selectProto(out, outlen, in, inlen, NGHTTP2_H2_16_ALPN) ||
          selectProto(out, outlen, in, inlen, NGHTTP2_H2_14_ALPN);
}

std::vector<unsigned char> getDefaultAlpn() 
{
   auto res = std::vector<unsigned char>(NGHTTP2_H2_ALPN.size() +
                                         NGHTTP2_H2_16_ALPN.size() +
                                         NGHTTP2_H2_14_ALPN.size());
   auto p = std::begin(res);

   p = std::copy_n(std::begin(NGHTTP2_H2_ALPN), NGHTTP2_H2_ALPN.size(), p);
   p = std::copy_n(std::begin(NGHTTP2_H2_16_ALPN), NGHTTP2_H2_16_ALPN.size(), p);
   p = std::copy_n(std::begin(NGHTTP2_H2_14_ALPN), NGHTTP2_H2_14_ALPN.size(), p);

   return res;
}

const unsigned char* getAlpnProtos(unsigned int &outLen)
{
   static const unsigned char sAlpnProtos[] = {
      2, 'h', '2',                        // "h2"
      8, 'h','t','t','p','/','1','.','1'  // "http/1.1"
   };
   outLen = sizeof(sAlpnProtos);
   return sAlpnProtos;
}

void configureTlsContext(asio::ssl::context &tlsContext)
{
   auto ctx = tlsContext.native_handle();

   auto ssl_opts = (SSL_OP_ALL & ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS) |
                  /*SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |*/
                  SSL_OP_NO_COMPRESSION |
                  SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION |
                  SSL_OP_SINGLE_DH_USE |
                  /*SSL_OP_SINGLE_ECDH_USE |*/
                  SSL_OP_NO_TICKET |
                  SSL_OP_CIPHER_SERVER_PREFERENCE;

  SSL_CTX_set_options(ctx, ssl_opts);
  SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);
  SSL_CTX_set_mode(ctx, SSL_MODE_RELEASE_BUFFERS);

  SSL_CTX_set_cipher_list(ctx, tls::DEFAULT_CIPHER_LIST);

#ifndef OPENSSL_NO_EC
   auto ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
   if (ecdh) 
   {
      SSL_CTX_set_tmp_ecdh(ctx, ecdh);
      EC_KEY_free(ecdh);
   }
#endif /* OPENSSL_NO_EC */

#ifndef OPENSSL_NO_NEXTPROTONEG
   //  NPN fallback for older clients 
   SSL_CTX_set_next_protos_advertised_cb(
      ctx,
      [](SSL *s, const unsigned char **data, unsigned int *len, void *arg) {
        auto &token = getAlpnToken();
        *data = token.data();
        *len = static_cast<unsigned int>(token.size());

        return SSL_TLSEXT_ERR_OK;
      },
      nullptr);
#endif // !OPENSSL_NO_NEXTPROTONEG

#if OPENSSL_VERSION_NUMBER >= 0x10002000L

   // Setup ALPN selection callback
   // Client can connect with h2 protocol even when we do not advertise ALPN
   // but we just want to make it explicit

   // Explicitly Advertise ALPN protocols to clients
   unsigned int len;
   auto alpnProtos = getAlpnProtos(len);   
   int rc = SSL_CTX_set_alpn_protos(ctx, alpnProtos, len );
   if (rc != 0) {
      throw std::runtime_error( "failed to set ALPN protocols");
   }

   // ALPN selection callback
   SSL_CTX_set_alpn_select_cb(ctx, alpnSelectProtoCb, nullptr);

#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L
}

bool tlsH2Negotiated( asio::ssl::stream<asio::ip::tcp::socket> &socket)
{
   auto ssl = socket.native_handle(); 
   const unsigned char *nextProto = nullptr;
   unsigned int nextProtoLen = 0;

#ifndef OPENSSL_NO_NEXTPROTONEG
   SSL_get0_next_proto_negotiated(ssl, &nextProto, &nextProtoLen);
#endif // !OPENSSL_NO_NEXTPROTONEG

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
   if (nextProto == nullptr) {
      SSL_get0_alpn_selected(ssl, &nextProto, &nextProtoLen);
   }
#endif // OPENSSL_VERSION_NUMBER >= 0x10002000L

   if (nextProto == nullptr && nextProtoLen == 0) {
      return false;
   }

  bool selected = checkH2IsSelected(StringRef{nextProto, nextProtoLen});
  bool valid = checkHttp2Requirement(ssl);

  return selected && valid;
}


} // namespace http2
} // namespace tbs

#endif //TOBASA_HTTP_USE_HTTP2