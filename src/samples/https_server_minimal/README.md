# HTTPS Server Minimal Sample

A minimal HTTPS server example using the Tobasa framework.

## Overview

This sample demonstrates a basic HTTPS server with minimal setup. It shows how to quickly get a secure web server running with TLS support using the `tobasahttp` library.

## Features

- HTTPS/TLS support
- Minimal configuration
- Basic request handling
- JSON responses

## Building

The sample is built as part of the main build system:

```bash
cmake -B build
cmake --build build
```

The compiled executable will be in `_output/https_server_minimal/debug/`

## Running

```bash
./https_server_minimal
```

The server will start and listen for HTTPS requests.

## Usage

Send HTTPS requests to the running server:

```bash
curl -k https://localhost:8443/
```

(Use `-k` to skip certificate verification in development)

## Endpoints

- `GET /` - Server status
- `GET /health` - Health check
- `POST /api/echo` - Echo request data back

## License

See LICENSE file in the root directory.
