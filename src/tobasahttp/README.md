 # Tobasa HTTP Library
C++ library for building HTTP servers and clients using Asio.

## Overview
Tobasa HTTP provides low-level HTTP/1.1 and HTTP/2 support, TLS, WebSocket
helpers, and request/response parsing. It focuses on network and protocol
handling so you can write request logic in your application.

## Features
- HTTP/1.1 server and client
- Optional HTTP/2 support (requires `nghttp2`)
- TLS/HTTPS support (OpenSSL)
- WebSocket utilities
- Asynchronous I/O using `asio`
- Multipart/form-data parsing and cookie helpers
- HTTP Compression

## Dependencies
- `tobasa` (core framework)
- `asio` (standalone Asio)
- `OpenSSL` (for TLS)
- `nghttp2` (optional, for HTTP/2)
- `zlib` (http compression)

## Notes
- The library provides protocol primitives; it is not a full web framework.
- For HTTP/2 enable `nghttp2` and build with its headers/libraries.
- For TLS enable OpenSSL and provide certificates via `settings_tls`.

## License
GNU LESSER GENERAL PUBLIC LICENSE

