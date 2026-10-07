# HTTP Response Compression

The server decides whether to gzip a response after your handler has built it. Your handler still sets the normal status, content type, and body. The server checks its compression settings, the request's `Accept-Encoding`, the response size and MIME type, and the response status. If the checks pass, it compresses the body while sending it.

## Configure gzip

Compression is enabled by default. For example, to raise the minimum body size and limit compression to selected types:

```cpp
http::SettingsTls settings;
settings
   .useCompression(true)
   .compressionMinimalLength(2048)
   .compressionEncoding("gzip")
   .compressionMimeTypes("text/plain text/css application/json");
```

The same settings are available on `http::Settings`. Set `useCompression(false)` to turn it off. `compressionMinimalLength()` cannot be set below 1024 bytes; smaller values are reset to 1024. The default threshold is 1024 bytes.

The default MIME list is `text/plain`, `text/css`, `application/text`, `application/json`, `application/javascript`, `text/xml`, `application/xml`, and `image/svg+xml`. `compressionMimeTypes()` takes a space-separated list and replaces the current list.

## What makes a response eligible

The server compresses only when all of these are true:

- compression is enabled in the server settings;
- the response body is at least the configured minimum size;
- the full response content-type string exactly matches an entry in the MIME list;
- the request's `Accept-Encoding` value contains `gzip`;
- the configured compression encoding contains `gzip`.

Gzip is the only encoding implemented. MIME matching is exact, not by type prefix. For example, `text/plain` does not match `text/plain; charset=utf-8` unless that full value is in the configured list. A body that passes the size threshold is compressed without first checking whether gzip will make it smaller.

There is a negotiation limitation in the current implementation: it uses a case-sensitive substring search for `gzip`; it does not parse `Accept-Encoding` tokens or quality weights. As a result, `gzip;q=0` still contains `gzip` and can enable compression, while `GZIP` does not match. Send a normal `Accept-Encoding: gzip` value if you want the current server to select gzip; do not rely on `q` values being honored.

## What goes on the wire

When gzip is selected, the response gets `Content-Encoding: gzip`. Compression is streamed, so the final compressed length is not known before sending. Do not set a `Content-Length` for a body the server will compress. For HTTP/1, the serializer uses chunked transfer encoding. For HTTP/2, the server ends the response stream when the body is complete; HTTP/2 does not use chunked transfer encoding.

The server does not add `Vary: Accept-Encoding`. If a cache can store the response, add `Vary: Accept-Encoding` yourself so compressed and uncompressed variants are not treated as the same representation.

## Responses that stay uncompressed

The response layer skips gzip for `206 Partial Content` and `416 Range Not Satisfiable`, so byte ranges continue to refer to the original file bytes. The Server-Sent Events response path also disables compression. Any response that fails an eligibility check is sent without gzip.

For resumable file downloads and range handling, see [http_server_resumable_download.md](http_server_resumable_download.md). The settings are declared in [server/settings.h](../include/tobasahttp/server/settings.h), and the response-side check is implemented in [response.cpp](../src/tobasahttp/response.cpp).
