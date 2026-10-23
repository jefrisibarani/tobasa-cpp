# HTTP TLS Setup Flow

`ConnectionStarterTls` configures TLS before accepting connections. It owns one
default TLS context and an `SniTlsContexts` object. When SNI hosts are
configured, the object also creates a separate context for each host.

## Default context

`setupTlsContext()` loads the default certificate chain and private key from
the configured files. If either file is absent, it asks the default TLS asset
callback for the corresponding in-memory bytes. DH parameters are loaded from
`tmpDhFile()` when present; otherwise, non-empty default DH bytes are used.
An encrypted default key can use the password from `privateKeyPassword()`.

Setup then calls `SniTlsContexts::configure()`. This installs the SNI callback
on the default context and creates the host-specific contexts. Each host uses
its configured certificate and key files, falling back to the default assets
when those files are absent. The configured DH file and default DH bytes are
also applied to each host context. A host-specific password callback is added
when its `HostCertificate` has a password.

During the TLS handshake, the callback looks up the client's SNI hostname. A
match switches that connection to the host's context. If the hostname is
missing or unknown, the default context remains selected. The callback also
saves the hostname for later access through `SniTlsContexts::hostname()`. Keep
the `SniTlsContexts` object alive while the default context can invoke its
callback.

## HTTP/2 negotiation

When built with `TOBASA_HTTP_USE_HTTP2` and enabled in settings,
`http2::configureTlsContext()` is applied to the default context and each
host-specific context. For `N` host-list entries, that is `N + 1` calls. The
helper sets TLS options, the cipher list, and protocol-negotiation callbacks.
ALPN is used when supported by the OpenSSL version; NPN is also enabled when
available in the OpenSSL build.

SNI chooses the certificate context, not the HTTP version. After the handshake,
`tlsH2Negotiated()` checks the negotiated protocol, requires TLS 1.2 or later,
and rejects blacklisted HTTP/2 ciphers. If those checks pass, the connection
uses HTTP/2; otherwise it uses HTTP/1. A build without HTTP/2 support always
uses HTTP/1.

See [TLS settings](../src/tobasahttp/doc/settings.md#tls-settings) and
[connection and request lifetime](../src/tobasahttp/doc/http_server_connection_and_request_lifetime.md#connection-and-request-lifetimes)
for the user-facing settings and protocol behavior.