# Serve Resumable File Downloads

The response keeps a file reader and, for a range request, selects a byte window from that file. This can let a client continue an interrupted download. Your application still chooses the file, checks access, and decides whether the file is the same version the client started downloading. Range support is opt-in for each response.

## Enable ranges for a file

Use `fileContent()` and enable ranges on the response:

```cpp
auto response = context->response();
response->fileContent(filePath);
response->enableFileRangeResponse(true);

response->setHeader("ETag", fileEtag); // Optional; needed only for If-Range validation.

response->httpStatus(http::StatusCode::OK);
return http::RequestStatus::handled;
```

`fileContent()` opens the file reader and records its size for this response. Keep the file available and unchanged until the response finishes; replacing or truncating it during a transfer can make the bytes sent disagree with the size and validators. The server does not lock or snapshot the file.

Range handling applies only when the option is enabled, the status is `200 OK`, the file reader opened successfully, and the response has no custom writer callback. It is disabled by default. The server applies the range before compression and serialization, for both HTTP/1 and HTTP/2. If your code needs to return a file error, check the file before building a success response; an unopened file reader is not made range-capable.

The ETag must identify this version of the file. Generate or store it in the application and change it when the file changes. The server does not generate ETags or `Last-Modified` headers. A normal `Range` request works without a validator; validators matter when the client sends `If-Range`.

## Range Requests

The client asks for bytes with a `Range` header. The server supports one byte range per request:

```http
Range: bytes=100-199
```

It also accepts an open end (`bytes=100-`) and a suffix (`bytes=-100`). For a valid satisfiable range, it returns `206 Partial Content`, sets `Content-Range`, and reads only that part of the file. An end beyond EOF is clamped to the final byte.

A valid range that starts beyond EOF returns `416 Range Not Satisfiable` with `Content-Range: bytes */<file-size>`. A zero-length suffix is also unsatisfiable. Malformed, unsupported, reversed, duplicate, or multi-range requests are ignored; the server sends the full file with `200 OK`. Range selection applies only to `GET`; `HEAD` and other methods get normal response behavior. When enabled for an open file response, the server advertises `Accept-Ranges: bytes`, even if the request has no `Range` header.

## Keep a resumed download on the same file version

For `If-Range`, the application must set an `ETag` or `Last-Modified` response header. The server does not create validators. Keep the validator stable while the file is unchanged and update it whenever the bytes change. The validator must describe the same representation used for the range offsets.

If `If-Range` is absent, the server applies a valid `Range` without checking a validator. If `If-Range` is present and matches, it applies the range. If the validator does not match, the response has no matching validator, or the request has duplicate `If-Range` headers, the server ignores the range and sends the full file.

The implementation compares ETag strings exactly and only matches a strong ETag. It compares `Last-Modified` values as exact strings; it does not parse or normalize the date. Send the same validator value that the server returned.

## Compression and byte offsets

The server disables compression for `206` and `416` responses. A full `200` response may still be compressed when the client accepts gzip. For reliable resume offsets, send `Accept-Encoding: identity` on the first request and every resumed request; otherwise the first response may be gzip-encoded while later ranges refer to the original file bytes. See [http_server_compression.md](http_server_compression.md).

`Response::prepareFileRangeResponse()` applies the range before compression and serialization. The implementation is tested in [http_range_response_test.cpp](../tests/unit/http_range_response_test.cpp).
