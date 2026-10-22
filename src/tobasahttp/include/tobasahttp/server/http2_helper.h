#ifdef TOBASA_HTTP_USE_HTTP2
#pragma once

/*
 * Adapted from nghttp2-asio
 * original file name: tls.h util.h
 *
 */

#include <asio/ssl.hpp>
#include <asio/ip/tcp.hpp>
#include <cstdio>
#include <stdexcept>
#include <vector>
#include <tobasa/util_string.h>
#include "tobasahttp/server/string_ref.h"
#include "tobasahttp/exp_imp.h"

namespace tbs {
namespace http2 {
namespace tls {
// Recommended general purpose "Intermediate compatibility" cipher
// suites for TLSv1.2 by mozilla.
// https://wiki.mozilla.org/Security/Server_Side_TLS
constexpr char DEFAULT_CIPHER_LIST[] =
    "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-"
    "AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-"
    "POLY1305:ECDHE-RSA-CHACHA20-POLY1305:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-"
    "AES256-GCM-SHA384";
} // namespace tls

constexpr auto NGHTTP2_H2_ALPN    = StringRef::from_lit("\x2h2");
constexpr auto NGHTTP2_H2         = StringRef::from_lit("h2");
// The additional HTTP/2 protocol ALPN protocol identifier we also
// supports for our applications to make smooth migration into final
// h2 ALPN ID.
constexpr auto NGHTTP2_H2_16_ALPN = StringRef::from_lit("\x5h2-16");
constexpr auto NGHTTP2_H2_16      = StringRef::from_lit("h2-16");
constexpr auto NGHTTP2_H2_14_ALPN = StringRef::from_lit("\x5h2-14");
constexpr auto NGHTTP2_H2_14      = StringRef::from_lit("h2-14");
constexpr size_t NGHTTP2_MAX_UINT64_DIGITS = util::strSize("18446744073709551615");

// Returns default ALPN protocol list, which only contains supported
// HTTP/2 protocol identifier.
TBS_HTTP_API std::vector<unsigned char> getDefaultAlpn();

// Returns true if ALPN ID |proto| is supported HTTP/2 protocol
// identifier.
TBS_HTTP_API bool checkH2IsSelected(const StringRef &proto);

// Selects h2 protocol ALPN ID if one of supported h2 versions are
// present in |in| of length inlen.  Returns true if h2 version is
// selected.
TBS_HTTP_API bool selectH2(const unsigned char **out, unsigned char *outlen, const unsigned char *in, unsigned int inlen);

TBS_HTTP_API const unsigned char* getAlpnProtos(unsigned int& outLen);

TBS_HTTP_API void configureTlsContext(asio::ssl::context &tlsContext);

TBS_HTTP_API bool tlsH2Negotiated( asio::ssl::stream<asio::ip::tcp::socket> &socket);

inline const std::vector<unsigned char>& getDefaultAlpnInModule()
{
    static const std::vector<unsigned char> protocols{
        2, 'h', '2',
        5, 'h', '2', '-', '1', '6',
        5, 'h', '2', '-', '1', '4'
    };
    return protocols;
}

inline void configureTlsContextInModule(asio::ssl::context& tlsContext)
{
    auto ctx = tlsContext.native_handle();
    auto sslOptions = (SSL_OP_ALL & ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS) |
                            SSL_OP_NO_COMPRESSION |
                            SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION |
                            SSL_OP_SINGLE_DH_USE |
                            SSL_OP_NO_TICKET |
                            SSL_OP_CIPHER_SERVER_PREFERENCE;

    SSL_CTX_set_options(ctx, sslOptions);
    SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);
    SSL_CTX_set_mode(ctx, SSL_MODE_RELEASE_BUFFERS);

    int cipherListResult = SSL_CTX_set_cipher_list(ctx, tls::DEFAULT_CIPHER_LIST);
#ifndef NDEBUG
    std::fprintf(
        stderr,
        "[tls_diag] OpenSSL=%s ctx=%p set_cipher_list=%d key_matches_certificate=%d min_version=0x%04x max_version=0x%04x\n",
        OpenSSL_version(OPENSSL_VERSION),
        static_cast<void*>(ctx),
        cipherListResult,
        SSL_CTX_check_private_key(ctx),
        SSL_CTX_get_min_proto_version(ctx),
        SSL_CTX_get_max_proto_version(ctx));
#endif

#ifndef OPENSSL_NO_EC
    auto ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (ecdh)
    {
        SSL_CTX_set_tmp_ecdh(ctx, ecdh);
        EC_KEY_free(ecdh);
    }
#endif

#ifndef OPENSSL_NO_NEXTPROTONEG
    SSL_CTX_set_next_protos_advertised_cb(
        ctx,
        [](SSL*, const unsigned char** data, unsigned int* len, void*) {
            auto& token = getDefaultAlpnInModule();
            *data = token.data();
            *len = static_cast<unsigned int>(token.size());
            return SSL_TLSEXT_ERR_OK;
        },
        nullptr);
#endif

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
    unsigned int len = 0;
    auto alpnProtos = getAlpnProtos(len);
    int result = SSL_CTX_set_alpn_protos(ctx, alpnProtos, len);
    if (result != 0)
        throw std::runtime_error("failed to set ALPN protocols");

    SSL_CTX_set_alpn_select_cb(
        ctx,
        [](SSL*, const unsigned char** out, unsigned char* outlen,
            const unsigned char* in, unsigned int inlen, void*) {
            return selectH2(out, outlen, in, inlen)
                ? SSL_TLSEXT_ERR_OK
                : SSL_TLSEXT_ERR_NOACK;
        },
        nullptr);
#endif
}

inline bool tlsH2NegotiatedInModule(asio::ssl::stream<asio::ip::tcp::socket>& socket)
{
    auto ssl = socket.native_handle();
    const unsigned char* protocol = nullptr;
    unsigned int protocolLength = 0;

#ifndef OPENSSL_NO_NEXTPROTONEG
    SSL_get0_next_proto_negotiated(ssl, &protocol, &protocolLength);
#endif

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
    if (!protocol)
        SSL_get0_alpn_selected(ssl, &protocol, &protocolLength);
#endif

    if (!protocol || protocolLength == 0 ||
         !checkH2IsSelected(StringRef{protocol, protocolLength}) ||
         SSL_version(ssl) < TLS1_2_VERSION)
        return false;

    auto cipher = SSL_get_current_cipher(ssl);
    if (!cipher)
        return false;

    int id = SSL_CIPHER_get_id(cipher) & 0xFFFFFF;
    return !((0x0000 <= id && id <= 0x00FF &&
                 ("\xFF\xFF\xFF\xCF\xFF\xFF\xFF\xFF\x7F\x00\x00\x00\x80\x3F\x00\x00"
                  "\xF0\xFF\xFF\x3F\xF3\xF3\xFF\xFF\x3F\x00\x00\x00\x00\x00\x00\x80"
                  [(id & 0xFF) / 8] & (1 << (id % 8)))) ||
                (0xC000 <= id && id <= 0xC0FF &&
                 ("\xFE\xFF\xFF\xFF\xFF\x67\xFE\xFF\xFF\xFF\x33\xCF\xFC\xCF\xFF\xCF"
                  "\x3C\xF3\xFC\x3F\x33\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                  [(id & 0xFF) / 8] & (1 << (id % 8)))));
}

} // namespace http2
} // namespace tbs

#endif // TOBASA_HTTP_USE_HTTP2