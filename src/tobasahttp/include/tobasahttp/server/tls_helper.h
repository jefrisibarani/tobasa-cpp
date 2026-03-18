#pragma once

#include <string>
#include <iostream>
#include <unordered_map>
#include "tobasahttp/server/settings_tls.h"

namespace tbs {
namespace http {

// Lookup function
SSL_CTX* lookupSslCtx(const std::string& hostname);

// SNI callback
int sniCallback(SSL* ssl, int* ad, void* arg);

void setupCertificateforSNI(
     const std::vector<HostCertificate>& list
   , const std::string& tmpDhFile
   , nonstd::span<const unsigned char> defaultCertChain
   , nonstd::span<const unsigned char> defaultPrivateKey
   , nonstd::span<const unsigned char> defaulTmpDh
#ifdef TOBASA_HTTP_USE_HTTP2
   , bool http2Enabled
#endif
);

} // namespace http
} // namespace tbs