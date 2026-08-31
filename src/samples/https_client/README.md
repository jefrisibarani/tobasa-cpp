# HTTPS Client Sample

A simple HTTPS client example using the Tobasa framework.

## Overview

This sample demonstrates how to make secure HTTPS requests to remote servers. It shows how to use the `tobasahttp` library to create a client with TLS/SSL support.

## Features

- HTTPS/TLS connection support
- HTTP/1.1 protocol
- JSON request and response handling
- Certificate validation

## Building

The sample is built as part of the main build system:

```bash
cmake -B build
cmake --build build
```

The compiled executable will be in `_output/https_client/debug/`

## Running

```bash
./https_client
```

## Usage Example

The client connects to a remote HTTPS endpoint and sends a request:

```bash
./https_client https://api.example.com/endpoint
```

## Features

- Supports GET and POST requests
- Handles JSON payloads
- Automatic TLS certificate verification
- Connection pooling


## License

See LICENSE file in the root directory.
