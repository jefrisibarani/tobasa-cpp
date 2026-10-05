# HTTPS Server Samples

This folder builds four small HTTPS server programs. They all listen on port
`8085` and use the `localhost.crt` and `localhost.key` certificate files. Run
only one at a time because they use the same port.

## Programs

| Target | Source file | What it does |
| --- | --- | --- |
| `server_with_worker` | `src/with_worker_threads.cpp` | Sends request work to a worker pool. It returns an HTML greeting containing the requested path. `/db` waits 15 seconds to simulate a slow database call. |
| `server_no_worker` | `src/without_worker_threads.cpp` | Handles each request directly and returns an HTML `Hello World!` page. Keep this handler short because it runs as part of I/O processing. |
| `server_minimal` | `src/minimal_io_context_thread.cpp` | Runs the I/O context on the main thread and returns plain-text `Hello World!` for every request. |
| `server_middlewares` | `src/with_middlewares.cpp` | Runs exception and multipart middleware before route handling. It includes a profile upload form. |

## Build

Build all four programs as part of the main project, or build one target from
the workspace root. For example:

```powershell
cmake --build build --target server_middlewares --config Debug
```

The sample's packaged executables are placed in
`_output/https_server_minimal/debug/` when package output is enabled.

## Run

In PowerShell, start one program from its output directory:

```powershell
cd _output/https_server_minimal/debug
.\server_middlewares.exe
```

Use `server_with_worker.exe`, `server_no_worker.exe`, or `server_minimal.exe`
to run one of the other programs. On Linux or macOS, use the same names without
`.exe` and prefix the command with `./`.

Open `https://localhost:8085/` or test it with:

```text
curl -k https://localhost:8085/
```

The `-k` option skips certificate verification. Use it only with this sample's
development certificate.

## Request handling

`server_with_worker` creates a worker pool sized from the machine's hardware
thread count. If that count is not available, it uses four workers. The request
handler returns `RequestStatus::async`, posts the work to the pool, and calls
`context->complete()` when the response is ready. The `/db` delay runs in the
worker pool, so it does not block I/O processing.

`server_no_worker` and `server_minimal` build their responses in the request
handler and return `RequestStatus::handled`. Keep this work quick. A long
operation in these handlers can delay other I/O work.

`server_middlewares` uses this order:

1. Exception middleware
2. Multipart middleware
3. Route handler

The multipart middleware defers parsing until it receives the request body. It
returns `RequestStatus::async` and continues to the route handler after parsing
finishes.

## Routes

The first three programs do not have general route tables. They return their
normal response for `/`, `/health`, and other paths. In `server_with_worker`,
the `/db` path also runs the 15-second delay.

`server_middlewares` has these routes:

- `GET /form_upload` shows a form with `User Name` and `Profile image` fields.
- `POST /upload` reads the `userName` and `profileImage` multipart fields. The
	response echoes the name and filename, and shows an inline preview for PNG,
	JPEG, GIF, and WebP uploads.
- Other paths show the default page with a link to the form.

The form route only accepts `GET`, and the upload route only accepts `POST`.
Other methods for those routes receive `405 Method Not Allowed`. Uploaded files
are written under `./tmp` while the request is processed.

## License

See the `LICENSE` file in the repository root.
