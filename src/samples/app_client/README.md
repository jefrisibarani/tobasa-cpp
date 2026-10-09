# App Client Sample


This sample demonstrates how to build a client that communicates with the app_server REST API. It shows how to make HTTP requests, handle responses, and work with JSON data.

## Features

- HTTP/HTTPS client
- REST API communication
- JSON serialization
- Authentication support

## Building

The sample is built as part of the main build system:
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

Configure the server connection in `appsettings.json`. With `verifyPeer: true`,
the client validates the server certificate chain and hostname using OpenSSL's
default CA paths. `caVerificationFile` is optional and adds a custom CA bundle
when the server uses a CA not present in those defaults. The sample config uses
`myCA.pem` for its local server certificate.

## License

See LICENSE file in the root directory.
