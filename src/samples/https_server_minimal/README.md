# HTTPS Server Minimal Sample

A small HTTPS server example using the Tobasa HTTP library. This sample shows
two ways to install a request handler:

- `src/with_worker_threads.cpp` sends request work to a worker pool;
- `src/without_worker_threads.cpp` handles the request on the I/O thread.

The CMake targets match these source files:

- `server_with_worker` is built from `with_worker_threads.cpp`;
- `server_no_worker` is built from `without_worker_threads.cpp`.

## Overview

This sample shows the basic server setup: create an `io_context`, configure a
TLS listener, install a request handler, start the server, and stop it when
`SIGINT` is received.

## Features

- HTTPS/TLS support
- Minimal configuration
- Basic request handling
- HTML responses
- Optional worker-pool processing

## Building

The sample is built as part of the main build system. It creates two
executables in `_output/https_server_minimal/debug/`:

- `server_with_worker`;
- `server_no_worker`.

## Running

```bash
./server_with_worker
./server_no_worker
```

Both executables listen for HTTPS requests on port `8085`.

Run one target at a time. They both bind to the same port, so they cannot run
at the same time.

## Usage

Send HTTPS requests to the running server:

```bash
curl -k https://localhost:8085/
```

(Use `-k` to skip certificate verification in development)

## The two request-handler examples

### `with_worker_threads.cpp` / `server_with_worker`: request handler with a worker pool

This source file is built as the `server_with_worker` target. It creates:

- an Asio I/O context for socket operations;
- an I/O thread count based on `std::thread::hardware_concurrency()`;
- an Asio worker pool for blocking request work.

The HTTP request handler starts the work and immediately returns:

```cpp
return tbs::http::RequestStatus::async;
```

The handler posts the actual work to `workerPool`. In the worker task,
`realRequestHandler()` builds the response. When the work is complete, it
posts back to the I/O context and calls:

```cpp
ctx->complete(resultStatus);
```

Calling `complete()` on the I/O context finishes the asynchronous request and
allows the server to send the response.

The `/db` path sleeps for 15 seconds to simulate a blocking database call. In
this version, that delay runs on the worker pool, so it does not block socket
processing on the I/O context. Other requests can continue to be accepted and
processed while the simulated database call is running, as long as a worker
thread is available.

The worker version also shows response features such as:

- the `X-Processed-By` response header;
- an HTML response body;
- a status code and content type;
- setting and removing cookies;
- sending an internal-server-error response when the worker task throws.

### `without_worker_threads.cpp` / `server_no_worker`: request handler without a worker pool

This source file is built as the `server_no_worker` target. It uses only the
Asio I/O context. The request handler writes the response directly and returns:

```cpp
return tbs::http::RequestStatus::handled;
```

It is shorter and useful when request work is quick and non-blocking. There is
no `asio::post()` to a worker pool and no later call to `context->complete()`.
The response is complete when the handler returns.

Do not perform slow database calls, file operations, or other blocking work in
this handler. A blocking call holds the I/O thread and can delay unrelated
connections. Use the worker-pool version when the handler may wait for a
blocking operation.

The worker-pool version has more moving parts because it must keep the request
asynchronous and return to the I/O context before completing it. The
no-worker version is easier to read, but it is only a good fit for short
handlers.

## Request handler choice

| Handler style | Use it when | Important rule |
| --- | --- | --- |
| With worker pool (`server_with_worker`, `with_worker_threads.cpp`) | The handler may call a blocking database, filesystem, or other slow API. | Return `RequestStatus::async`, do the work on the worker pool, then call `context->complete()` on the I/O context. |
| Without worker pool (`server_no_worker`, `without_worker_threads.cpp`) | The handler only does short, non-blocking work. | Build the response and return `RequestStatus::handled`; do not block the I/O thread. |

## Endpoints

Both examples answer every request with an HTML response. There is no router
and no separately registered `/health` endpoint.

- `GET /` - The worker version returns the requested path in the response;
	the no-worker version returns `Hello World!`.
- `/health` - This is not a special endpoint. It receives the same default
	response as any other path.
- `/db` - In `with_worker_threads.cpp`, simulates a 15-second blocking
	database call on the worker pool before returning the response.

The `without_worker_threads.cpp` version does not include the `/db`
simulation.

## License

See LICENSE file in the root directory.
