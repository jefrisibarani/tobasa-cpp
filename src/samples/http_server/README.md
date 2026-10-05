# HTTP Server Sample

This sample starts an HTTP server and an HTTPS server. Both use the same
request handler and WebSocket context.

The sample shows how to:

- serve files from `wwwroot`;
- handle a multipart file upload;
- save and browse uploaded files;
- handle WebSocket connections;
- configure HTTP and HTTPS server settings.

## Build

Build the `http_server` target from the workspace root:

```powershell
cmake --build build --target http_server --config Debug
```

CMake copies `wwwroot` and the TLS files beside the executable. It also copies
timezone data when the build is configured to use external timezone data. If
package output is enabled, it copies the executable and resources to
`_output/http_server/debug/` for a Debug build.

## Run

Run the executable from the folder that contains it and the copied resources.
In PowerShell:

```powershell
.\http_server.exe
```

The servers listen on all network interfaces:

- HTTP: `http://localhost:8084`
- HTTPS: `https://localhost:8085`

The HTTPS certificate is for local testing. Use `-k` with `curl` to skip
certificate verification:

```text
curl.exe http://localhost:8084/
curl.exe -k https://localhost:8085/
```

Press `Ctrl+C` to stop both servers. The sample stops the servers and releases
the WebSocket context before it stops the I/O context.

## Request handling

The sample creates one `asio::io_context` and runs it on four threads. The HTTP
and HTTPS servers use the same function, `handleServerRequest()`, to choose
what to do with each request:

| Path | What the handler does |
| --- | --- |
| `/hello` | Returns a small HTML greeting and demonstrates response headers and cookies. |
| `/upload` | Handles a multipart upload or returns a non-multipart request body in an HTML page. |
| `/browse_uploads` and `/browse_uploads/...` | Lists saved upload folders and files, or returns a requested file. |
| `/websocket_ep` | Starts a WebSocket connection. |
| `/test_websocket` | Serves the WebSocket browser test page. |
| Any other path | Reads the matching file from `wwwroot`. A path ending in `/` uses `index.html`. |

The handler selects these actions by checking the request path. It does not use
a separate router. Only `/upload` checks the HTTP method and requires `POST`;
the other paths are not restricted to a particular method by this handler.

## Uploads

Open `http://localhost:8084/test_multipart.html` in a browser to use the upload
form. The form sends two files to `/upload`:

- `profileImage`, which the server returns in the response;
- `attachment`, which the server also saves.

It also sends `userName` and `userNote`. For a multipart request with a
`profileImage` file, the server copies each uploaded file into a new,
timestamp-named folder under `app_data/uploads` beside the executable. It writes
`userName` and `userNote` to an `info.json` file in that folder. File names are
cleaned before saving, and duplicate names get a numeric suffix.

The response sends the saved `profileImage` file and uses the content type from
that multipart part. Multipart temporary files are stored in `./tmp` while
the request is processed.

You can also send an upload with `curl`:

```text
curl.exe -F "userName=Sam" -F "userNote=Test upload" -F "profileImage=@image.png" -F "attachment=@document.pdf" http://localhost:8084/upload
```

## Browse uploaded files

Open `http://localhost:8084/browse_uploads` to see saved uploads. The page lets
you open folders and files. A file request returns that file with a content
type chosen from its extension.

Uploads are stored under:

```text
<executable folder>/app_data/uploads/<timestamp>/
```

The browser maps `/browse_uploads/` to that folder. It checks paths stay inside
the upload folder and does not show symbolic links. Missing paths return `404`
and paths outside the folder return `403`.

## WebSockets

Open `http://localhost:8084/test_websocket` to load the browser test page. It
connects to the `/websocket_ep` WebSocket endpoint over HTTPS by default. The
page's endpoint field can be changed when needed.

When a client connects, the server sends a welcome message with the connection
ID and a user ID. Send messages in this format:

```text
MESSAGE|{destination}|{data}
```

Use a numeric connection ID to send to one client, or use `ALL` to broadcast to
all clients. Other text is echoed to the sender with an `[echo]` prefix. Invalid
message formats and destinations receive an error message.

## `wwwroot` files

The current web root contains these files:

| URL | File |
| --- | --- |
| `/` or `/index.html` | `wwwroot/index.html`, the home page. |
| `/test_multipart.html` | `wwwroot/test_multipart.html`, the upload form and response viewer. |
| `/test_websocket.html` | `wwwroot/test_websocket.html`, the WebSocket test page. |
| `/css/styles.css` | `wwwroot/css/styles.css`, the page styles. |
| `/js/tbs.js` | `wwwroot/js/tbs.js`, the sample JavaScript file. |
| `/assets/images/...` | `_default_doctor.jpg`, `_default_person.jpg`, `_default_person.png`, `_default_slide.jpg`, `_logo.jpg`, `_logo_report.jpg`, and `index.html`. |

The static-file handler blocks paths containing `..`, checks that resolved paths
stay inside `wwwroot`, and returns `404` for missing files. It chooses the
response content type from the file extension.

## Server settings

| Setting | HTTP | HTTPS |
| --- | --- | --- |
| Address | `0.0.0.0` | `0.0.0.0` |
| Port | `8084` | `8085` |
| Read timeout | 10 seconds | 10 seconds |
| Write timeout | 60 seconds | 60 seconds |
| Request processing timeout | 3600 seconds | 3600 seconds |
| Read and send buffers | 32 KB | 32 KB |
| Maximum header size | 1 MB | 1 MB |
| Multipart parsing | Enabled | Enabled |
| Multipart temporary folder | `./tmp` | `./tmp` |
| Maximum requests per connection | No limit | 100 |

HTTPS uses `localhost.crt`, `localhost.key`, and `dh2048.pem`. HTTP/2 is
disabled by this sample, even when HTTP/2 support is compiled into the library.

## Source files

- [`src/main.cpp`](src/main.cpp) sets up both servers and handles requests.
- [`src/server_lib.cpp`](src/server_lib.cpp) contains upload folder and file-name helpers.
- [`src/server_lib.h`](src/server_lib.h) declares those helpers.
- [`wwwroot/`](wwwroot/) contains the sample pages and assets.
- [`CMakeLists.txt`](CMakeLists.txt) copies the web root and TLS resources beside the executable.

## License

See [LICENSE](../../../LICENSE) in the repository root.
