# App Client Sample

A client application example that connects to the Tobasa app server.

## Overview

This sample demonstrates how to build a client that communicates with the app_server REST API. It shows how to make HTTP requests, handle responses, and work with JSON data.

## Features

- HTTP/HTTPS client
- REST API communication
- JSON serialization
- Authentication support

## Building

The sample is built as part of the main build system:

```bash
cmake -B build
cmake --build build
```

The compiled executable will be in `_output/app_client/debug/`

## Running

First, start the app_server:

```bash
./app_server
```

Then run the client:

```bash
./app_client
```

## Usage

The client connects to the app_server and performs various API requests:

- User authentication
- Data retrieval
- Resource management
- WebSocket connections

## Configuration

Configure the server connection in `appsettings.json`:

## Dependencies

- Tobasa core library
- tobasahttp library
- nlohmann JSON library

## License

See LICENSE file in the root directory.
