# Building Tobasa C++ 

This document provides comprehensive instructions for building the Tobasa C++ framework and its components.

## Prerequisites

### Required Tools

- **CMake 3.12-3.18**: Build system generator
- **Git**: For cloning the repository (if not already done)

### Compilers

#### Windows
- **Visual Studio 2017** or **Visual Studio 2022** (MSVC)
- **VCPKG** (optional, for dependency management)

#### Linux (Ubuntu Server 20.04.5)
- **GCC 9.4.0** or **GCC 11.1.0**

### Dependencies

Tobasa requires several third-party libraries. You can either:

1. **Use VCPKG** (Windows only): Automatic dependency management
2. **Use prebuilt libraries**: Download and configure paths in `CMakeDefaults.txt`
3. **Build from source**: Compile dependencies manually

#### Required Libraries

- **OpenSSL**: TLS/SSL support
- **Zlib**: Compression
- **SQLite**: Database support
- **MariaDB/MySQL**: Database support
- **PostgreSQL**: Database support
- **Protobuf**: Protocol buffer support (optional)
- **nghttp2**: HTTP/2 support (optional)

## Quick Start

### Windows

Run the provided build script from the project root:

```cmd
.\build_all.cmd
```

This will:
1. Create a `build` directory
2. Configure CMake for Visual Studio 2017 (x64)
3. Build both Release and Debug configurations

### Linux

Run the provided build script from the project root:

```bash
./build_all.sh
```

This will:
1. Create `build_d` (Debug) and `build_r` (Release) directories
2. Configure CMake with Unix Makefiles
3. Build both configurations

## Manual Build Process

### Step 1: Prepare Dependencies

#### Option A: Using VCPKG (Windows)

1. Install VCPKG:
   ```cmd
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   ```

2. Set VCPKG paths in `CMakeDefaults.txt`:
   ```cmake
   set(VCPKG_TOOLCHAIN_FILE_DEF "d:/vcpkg/scripts/buildsystems/vcpkg.cmake")
   set(VCPKG_OVERLAY_PORTS_DEF "d:/vcpkg/my_ports_vs2017")  # or vs2022
   set(VCPKG_TARGET_TRIPLET_DEF "x64-windows-static-vs2017")  # or vs2022
   ```

3. Enable VCPKG in CMake:
   ```cmake
   -DTOBASA_BUILD_WITH_VCPKG=ON
   ```

#### Option B: Prebuilt Libraries (Windows)

Download and extract prebuilt libraries to the paths specified in `CMakeDefaults.txt`:

- OpenSSL: `d:/dev/openssl_x64-windows-static-vs2017`
- Zlib: `d:/dev/dcmtk/dcmtk_3.6.8_x64_MT_vs2017/zlib-1.3`
- PostgreSQL libpq: `d:/dev/libpq_x64-windows-static-vs2017`
- Protobuf: `d:/dev/protobuf_x64-windows-static-vs2017`
- nghttp2: `d:/dev/nghttp2_x64-windows-static-vs2017`

#### Option C: System Packages (Linux)

Install via package manager:

```bash
sudo apt-get update
sudo apt-get install libssl-dev zlib1g-dev libsqlite3-dev \
                     libmariadb-dev libpq-dev protobuf-compiler \
                     libprotobuf-dev libnghttp2-dev
```

### Step 2: Configure Build

Create a build directory and run CMake configuration:

#### Windows (Visual Studio)

```cmd
mkdir build
cd build
cmake -S .. -G "Visual Studio 15 2017" -T host=x64 -A x64
```

For Visual Studio 2022:
```cmd
cmake -S .. -G "Visual Studio 17 2022" -T host=x64 -A x64
```

#### Linux (GCC)

```bash
mkdir build
cd build
cmake -S .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
```

### Step 3: Build

#### Windows

```cmd
cmake --build . --config Debug
cmake --build . --config Release
```

#### Linux

```bash
cmake --build .
```

## Build Options

Tobasa provides several CMake options to customize the build:

### Core Options

- `TOBASA_BUILD_WITH_VCPKG`: Use VCPKG for dependency management (Windows only, default: OFF)
- `TOBASA_BUILD_IN_MEMORY_RESOURCES`: Embed resources in binary (default: ON)
- `TOBASA_BUILD_IN_MEMORY_TZDB`: Embed timezone data in binary (default: ON)

### Database Drivers

- `TOBASA_SQL_USE_SQLITE`: Enable SQLite support (default: ON)
- `TOBASA_SQL_USE_MYSQL`: Enable MariaDB/MySQL support (default: ON)
- `TOBASA_SQL_USE_PGSQL`: Enable PostgreSQL support (default: ON)
- `TOBASA_SQL_USE_ODBC`: Enable ODBC support (default: ON)
- `TOBASA_SQL_USE_ADODB`: Enable ADO support (default: ON)

### Optional Components

- `TOBASA_BUILD_LIS_ENGINE`: Build LIS (Lab Instrument) support (default: ON)
- `TOBASA_BUILD_TESTS`: Build test modules (default: ON)
- `TOBASA_BUILD_TESTS_WSCHAT_USE_PROBUF`: Build WebSocket tests with protobuf (default: ON)
- `TOBASA_HTTP_USE_HTTP2`: Enable HTTP/2 support (default: ON)

### Example Custom Configuration

```bash
cmake -S . -B build \
  -DTOBASA_BUILD_TESTS=OFF \
  -DTOBASA_BUILD_LIS_ENGINE=OFF \
  -DTOBASA_HTTP_USE_HTTP2=OFF \
  -DCMAKE_BUILD_TYPE=Release
```

## Output Structure

After successful build, the output is organized as follows:

```
_output/
├── dyn_lib_sample/
│   └── debug/          # Dynamic library samples
├── http_server/
│   └── debug/          # HTTP server sample
├── https_client/
│   └── debug/          # HTTPS client sample
├── https_server_minimal/
│   └── debug/          # Minimal HTTPS server
├── test_tobasa/
│   └── debug/          # Core library tests
├── test_tobasasql/
│   └── debug/          # SQL library tests
├── webclient/
│   └── debug/          # Web client demo
└── webservice/
    └── debug/          # Main web service application
```

## Build Targets

Available CMake targets:

- `ALL_BUILD`: Build all components
- `webservice`: Main web service application
- `webclient`: HTTP client demo
- `test_tobasa`: Core library tests
- `test_tobasasql`: SQL library tests
- `dyn_lib_sample`: Dynamic library sample
- `http_server`: HTTP server sample
- `https_client`: HTTPS client sample
- `https_server_minimal`: Minimal HTTPS server

## Troubleshooting

### Common Issues

#### CMake Configuration Errors

**Problem**: CMake cannot find dependencies
**Solution**:
- Verify paths in `CMakeDefaults.txt`
- Ensure libraries are built with matching compiler version
- Check that library directories contain expected files

#### Linker Errors

**Problem**: Missing symbols or unresolved externals
**Solution**:
- Ensure all required libraries are installed
- Check that library versions match (32-bit vs 64-bit)
- Verify runtime library settings (MT vs MD)

#### Compiler Errors

**Problem**: C++ standard or feature support issues
**Solution**:
- Ensure compiler supports C++17
- Update to supported Visual Studio or GCC version

### Debug Build

For debugging issues, build with debug symbols:

```bash
# Windows
cmake --build . --config Debug

# Linux
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Verbose Output

Enable verbose CMake output:

```bash
cmake -S . -B build --log-level=VERBOSE
```

### Clean Build

To start fresh:

```bash
# Remove build directory
rm -rf build/

# Remove output directory
rm -rf _output/
```

## Advanced Configuration

### Custom Library Paths

Edit `CMakeDefaults.txt` to customize dependency paths:

```cmake
# Example custom paths
set(TOBASA_OPENSSL_DIR_DEF "C:/custom/openssl")
set(TOBASA_ZLIB_DIR_DEF "C:/custom/zlib")
```

### IDE Integration

#### Visual Studio
- Open `build/TobasaFramework.sln`
- Build from within Visual Studio

#### VS Code
- Use CMake Tools extension
- Configure with `cmake.configureSettings`

## Performance Considerations

- **Release builds**: Use `-DCMAKE_BUILD_TYPE=Release` for optimized binaries
- **Parallel builds**: Use `cmake --build . -j <num_cores>` for faster compilation
- **Incremental builds**: Only rebuild changed files

## Getting Help

- Check individual component README files for specific build instructions
- Review CMake output for detailed error messages
- Ensure all prerequisites are correctly installed
- Verify compiler and library versions match requirements</content>
<parameter name="filePath">d:\projects\tobasa_cxx_foss\BUILD.md