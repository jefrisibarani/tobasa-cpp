
# Tobasa C++ Libraries

Tobasa is a collection of small, focused C++ libraries that form the
building blocks web‑service applications.\
Each sub‑project is self‑contained; you can link only the libraries you need.

## Projects in this repository

| Module         | Description |
|----------------|-------------|
| `tobasa`       | **Common library** – utilities such as logging, configuration, JSON,
|                | date/time, string helpers, plugins, task manager, etc. 
|                | This is the foundation for all other Tobasa components. |
| `tobasahttp`   | **HTTP library** – Asio‑based HTTP/1.1 and HTTP/2(with nghttp) server and client with
|                | optional TLS, WebSocket, multipart parsing, etc. |
| `tobasaweb`    | **Web App/REST framework** – routing, controllers, middleware, JWT/session
|                | support built on top of `tobasahttp` and `tobasasql`. |
| `tobasasql`    | **SQL abstraction** – lightweight wrapper around SQLite/MySQL/Postgres
|                | and MSSQL(through ODBC and ADO) with connection pools, query helpers. |
| `tobasalis`    | **LIS (Lab Instrument) library** – message parsing (LIS2‑A2/ASTM and HL7), 
|                | TCP/serial transports, session management. |
| `webservice`   | **Example web service application** – integrates the above libraries
|                | into a runnable server with configuration and database helpers. |
| `webclient`    | **HTTP client demo** – simple client using `tobasahttp`.



## Repository layout

```
src/                   
  app_server/         # sample webservice application
  ext/                # bundled third-party libs 
  samples/            # samples projects
  tobasa/             # core helpers library
  tobasahttp/         # HTTP transport library
  tobasaweb/          # Web App framework library
  tobasasql/          # SQL abstraction library
  tobasalis/          # LIS protocol support  library
CMakeLists.txt        # top-level CMake project
CMakeDefaults.txt     # machine-specific default paths
build/                # out-of-source build directory
_output/              # packaged build result
```

## Build
Tobasa uses CMake as the build system. The project is currently tested with
MSVC on Windows and GCC on Linux.

### Windows
- Visual Studio 2017
- Visual Studio 2022

### Ubuntu Server 20.04.5
- GCC 9.4.0
- GCC 11.1.0

See [build_all.cmd](./build_all.cmd) and [build_all.sh](./build_all.sh) for example\
See also [BUILD](./BUILD.md) for more information 


## License

Tobasa libraries are licensed under the GNU LGPL (Lesser General Public License).\
Tobasa applications are licensed under the GNU GPL (General Public License).

---

(See also each subdirectory’s README for detailed information on that
module.)

