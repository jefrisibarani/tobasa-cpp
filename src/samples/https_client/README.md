# HTTPS Client Sample

This sample sends HTTPS requests to a local server at `127.0.0.1:8085` using
`SecureClientDefault`.

## Features

- HTTPS/TLS connection support
- HTTP/1.1 protocol
- JSON request and response handling
- Peer certificate validation is disabled in this sample

## Building

The sample is built as part of the main build system:
The compiled executable will be in `_output/https_client/debug/`

## Running

```bash
./https_client
```

## Usage Example

Start an HTTPS server on `127.0.0.1:8085`, then run the sample. It sends GET
requests to `/api/version` and a POST request to `/api/users/authenticate`.

In `src/main.cpp`, `verifyPeer(false)` leaves TLS encryption enabled but skips
server certificate and hostname validation. To enable validation, set
`verifyPeer(true)`. OpenSSL's default CA paths are used; set
`caVerificationFile()` only when you need to trust an additional CA or use a
custom CA bundle. The configured server name must match the certificate.


## License

See LICENSE file in the root directory.
