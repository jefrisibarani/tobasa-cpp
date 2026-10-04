# HTTP Parser

## Overview

`tbs::http::parser::Parser` parses one HTTP/1.x message: either a request or a response. It processes input incrementally, so the caller can pass each newly received buffer to `parse()` while the parser retains its progress between calls. It handles the start line, headers, message body framing, and optionally multipart form data. It does not handle routing or application behavior.

The parser is declared in [`http_parser.h`](../include/tobasahttp/http_parser.h) and implemented in [`http_parser.cpp`](../src/tobasahttp/http_parser.cpp).

## How Parsing Works

1. The caller constructs a `Parser` with a message type and a reference to its input buffer. The buffer is owned by the caller, and must remain valid for the parser's lifetime.
2. Each time more bytes arrive, the caller invokes `parse(bytesTransferred)`. The parser remembers partial start lines, headers, and body state across calls.
3. After the headers are complete, the parser determines how the body ends. It reads a body with `Content-Length` until that many bytes arrive, decodes `Transfer-Encoding: chunked` until the final chunk and trailers are consumed, or completes immediately when there is no body.
4. If the content type is `multipart/form-data` and multipart parsing is enabled, the parser uses the boundary to parse the form fields and files as body data arrives.
5. Each call returns an `Info` describing success or failure and how much of the current input was processed. The caller continues reading when the message is incomplete, handles errors when parsing fails, and dispatches the completed message when parsing is done.

`done()` is true when both the headers and message body are complete. `headersDone()` and `contentDone()` let callers check those stages separately.

## Lifetime on a Connection

The server creates one `Parser` as part of each `HttpConnection`. The parser holds a reference to that connection's read buffer; it is not reconstructed for every request.

For a keep-alive connection, after a response is sent, `ServerConnection::handleKeepAliveOrClose()` calls `prepareForNextMessage()`, assigns the next request ID, and starts reading again. `prepareForNextMessage()` clears the previous message's headers, body, framing state, multipart state, and parsing progress, while preserving the parser's connection-level configuration and its reference to the read buffer. The same parser object then parses the next request.

If the connection is not kept alive, the server closes it and the parser is destroyed with the connection. The client connection also reuses its parser for successive responses and resets it with `prepareForNextMessage()` when continuing.

## Results and Callbacks

`Info` reports whether parsing succeeded, an error message and HTTP status when relevant, the last input index, and the number of bytes read. Callers should use these values to decide whether to read more data, report an error, or continue processing.

After parsing completes, `headers()`, `content()`, and `multipartBody()` provide the parsed results. The body accessors transfer their stored values out of the parser; retrieve each result only when needed and only once.

The parser also provides callbacks for request-method validation, header validation, and completed parsing stages. A server can use these hooks to apply its own policy or observe when headers, body, or multipart parsing completes.

## Framing and Multipart Notes

- A message must not contain both `Content-Length` and `Transfer-Encoding: chunked`; the parser rejects that combination.
- `Content-Length` describes the HTTP body size. For multipart data, boundaries divide that body into form parts; they are a separate layer of framing.
- Internal multipart parsing is enabled by default in the parser and HTTP server settings. The server can disable it to hand body reading to middleware instead. See [internal multipart parsing](parsing_multipart_internally.md) and [multipart middleware parsing](parsing_multipart_with_middleware.md) for those flows.
- The configured maximum header size defaults to 8 KiB. The request-target limit is currently 4 KiB.
- The parser accepts HTTP/1.0 and HTTP/1.1 start lines. Application code remains responsible for deciding whether a parsed request is allowed and what response to send.
