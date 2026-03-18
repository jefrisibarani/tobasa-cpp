#ifdef TOBASA_HTTP_USE_HTTP2
#pragma once

/*
 * Adapted from nghttp2-asio
 * original file name: tls.h util.h
 *
 */

#include <asio/ssl.hpp>
#include <asio/ip/tcp.hpp>
#include <tobasa/util_string.h>
#include "tobasahttp/server/string_ref.h"

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
std::vector<unsigned char> getDefaultAlpn();

// Returns true if ALPN ID |proto| is supported HTTP/2 protocol
// identifier.
bool checkH2IsSelected(const StringRef &proto);

// Selects h2 protocol ALPN ID if one of supported h2 versions are
// present in |in| of length inlen.  Returns true if h2 version is
// selected.
bool selectH2(const unsigned char **out, unsigned char *outlen, const unsigned char *in, unsigned int inlen);

const unsigned char* getAlpnProtos(unsigned int& outLen);

void configureTlsContext(asio::ssl::context &tlsContext);

bool tlsH2Negotiated( asio::ssl::stream<asio::ip::tcp::socket> &socket);

} // namespace http2
} // namespace tbs

#endif // TOBASA_HTTP_USE_HTTP2