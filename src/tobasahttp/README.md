 # Tobasa HTTP Library

A high-performance C++ library for building HTTP/HTTPS servers and clients with async I/O support.

## Overview

Tobasa HTTP provides production-ready HTTP/1.1 and HTTP/2 protocol support built on Asio. It handles low-level network I/O, protocol parsing, TLS encryption, and WebSocket communication, allowing you to focus on application logic.

## Features

**Core Protocol Support:**
- HTTP/1.1 server and client with keep-alive
- HTTP/2 support (optional, with nghttp2)
- HTTPS/TLS encryption (OpenSSL)
- WebSocket protocol utilities
- Chunked transfer encoding
- Content compression (gzip, deflate)

**Request/Response Handling:**
- Multipart/form-data parsing
- Cookie parsing and management
- Header field validation
- Query string parsing
- JSON content-type handling

**Performance & Scalability:**
- Asynchronous I/O using Asio
- Connection pooling
- Non-blocking event loop
- Suitable for REST APIs and embedded servers

## Building

The library is built as part of the main build system:

```bash
cmake -B build
cmake --build build
```

Enable optional features with CMake flags:
```bash
cmake -B build -DENABLE_HTTP2=ON -DENABLE_COMPRESSION=ON
cmake --build build
```

## Usage

Tobasahttp is used by higher-level frameworks like `tobasaweb` for routing and middleware. For direct use, see the samples in `src/samples/http_server/` and `src/samples/https_client/`.

## Dependencies

- `tobasa` - Core framework
- `asio` - Asynchronous I/O library
- `OpenSSL` - For HTTPS/TLS support
- `nghttp2` (optional) - For HTTP/2 protocol
- `zlib` (optional) - For compression

## Architecture

Tobasahttp provides transport-layer primitives:
- Parser classes for HTTP messages
- Server and client connection managers
- TLS certificate/key handling
- Buffer management for efficient I/O

Higher-level routing, middleware, and controllers are handled by `tobasaweb` framework.

## Performance Notes

- Built for low-latency, high-throughput applications
- Efficient protocol parsing using state machines
- Connection pooling reduces overhead
- Minimal memory allocations in hot paths

## License

GNU LESSER GENERAL PUBLIC LICENSE

