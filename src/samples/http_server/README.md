# HTTP Server Sample

A simple HTTP server example using the Tobasa framework.

## Overview

This sample demonstrates a basic HTTP server that listens for incoming requests and returns responses. It shows how to use the `tobasahttp` library to create a functional web server.

## Features

- HTTP/1.1 support
- HTTP/2 support (optional, with nghttp2)
- Simple request routing
- JSON response handling
- Basic error handling

## Building

The sample is built as part of the main build system:

```bash
cmake -B build
cmake --build build
```

The compiled executable will be in `_output/http_server/debug/`

## Running

```bash
./http_server
```

The server will start and listen for HTTP requests on the default port.

## Usage

Send HTTP requests to the running server. Example:

```bash
curl http://localhost:8084/
```

## Example Endpoints

- `GET /` - Returns status information
- `GET /api/version` - Returns API version
- `POST /api/data` - Accepts JSON data and echoes it back

## License

See LICENSE file in the root directory.
