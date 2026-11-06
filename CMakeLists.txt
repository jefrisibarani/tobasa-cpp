cmake_minimum_required(VERSION 3.13-3.18)
include(CMakeDependentOption)
cmake_policy(SET CMP0091 NEW)


# When OFF, build Tobasa as static libraries and link against the static MSVC runtime
option(TOBASA_BUILD_SHARED_LIB 
      "Build Tobasa as a shared library and use the dynamic MSVC runtime" OFF)

# Build Tobasa as single shared library
CMAKE_DEPENDENT_OPTION(TOBASA_BUILD_MONOLITHIC_SHARED_LIB
      "Build Tobasa modules into one shared library" OFF
      "TOBASA_BUILD_SHARED_LIB" OFF)

# Database Drivers
option(TOBASA_SQL_USE_PGSQL      "Enable PostgreSQL driver"           ON )
option(TOBASA_SQL_USE_ADODB      "Enable ADO driver"                  ON )
option(TOBASA_SQL_USE_ODBC       "Enable ODBC driver"                 ON )
option(TOBASA_SQL_USE_SQLITE     "Enable SQLite driver"               ON )
option(TOBASA_SQL_USE_MYSQL      "Enable MariaDB driver"              ON )

option(TOBASA_HTTP_USE_HTTP2     "Enable HTTP/2 support"              ON )
option(TOBASA_BUILD_LIS_ENGINE   "Build LIS Engine"                   ON )

option(TOBASA_BUILD_TESTS        "Build Test Module"                  ON )
option(TOBASA_BUILD_TESTS_SQLDB_INTEGRATION
                                 "Build database integration tests"   ON )

option(TOBASA_BUILD_TESTS_WSCHAT_USE_PROBUF 
                                 "Build Test Websocket with protobuf" ON )

CMAKE_DEPENDENT_OPTION(TOBASA_BUILD_TESTS_WSCHAT_USE_PROBUF 
      "Build Test Websocket with protobuf" ON
      "TOBASA_BUILD_TESTS" OFF )

option(TOBASA_BUILD_IN_MEMORY_RESOURCES "Whether to use compiled resources"              ON )
option(TOBASA_BUILD_IN_MEMORY_TZDB      "Whether to use compiled timezone data"          ON )

# Valid only when using the MSVC compiler
option(TOBASA_BUILD_WITH_VCPKG "Build Tobasa with VCPKG libraries"                       OFF )
option(TOBASA_FIND_DEPS        "Use `find_package()` for including Tobasa dependencies." OFF)


# -------------------------------------------------------
# External library deps location
# Must included before project(), because 
# CMAKE_TOOLCHAIN_FILE must be defined before the first project() call and before CMake starts configuring
# so we have correct path for required libs when building with MSVC or VCPKG
include("CMakeDefaults.txt")

if(TOBASA_BUILD_WITH_VCPKG)
   set(VCPKG_TARGET_TRIPLET ${VCPKG_TARGET_TRIPLET_DEF} CACHE STRING "VCPKG triplet to use" )
   set(VCPKG_OVERLAY_PORTS  ${VCPKG_OVERLAY_PORTS_DEF}  CACHE STRING "VCPKG custom port folder" )
   set(VCPKG_TOOLCHAIN_FILE ${VCPKG_TOOLCHAIN_FILE_DEF} CACHE STRING "VCPKG toolchain file" )
   set(CMAKE_TOOLCHAIN_FILE ${VCPKG_TOOLCHAIN_FILE})
endif() 


set(TOBASA_PACKAGE_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/_output CACHE STRING "Location of package directory" )
set(TOBASA_CREATE_PACKAGE_OUTPUT_DIR ON CACHE BOOL "Create a package directory bundle" )

# -------------------------------------------------------
# Global MSVC runtime to the selected library type.
if(TOBASA_BUILD_SHARED_LIB)
   set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
else()
   set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
# -------------------------------------------------------
if(TOBASA_BUILD_MONOLITHIC_SHARED_LIB)
   if(POLICY CMP0079)
      cmake_policy(SET CMP0079 NEW)
   else()
      message(FATAL_ERROR "TOBASA_BUILD_MONOLITHIC_SHARED_LIB requires CMake 3.13 or newer")
   endif()
endif()
# -------------------------------------------------------
if(TOBASA_BUILD_SHARED_LIB AND TOBASA_SQL_USE_MYSQL)
   set(TOBASA_SQL_USE_MARIADB_DLL ON CACHE BOOL "" FORCE)
else()
   set(TOBASA_SQL_USE_MARIADB_DLL OFF CACHE BOOL "" FORCE)
endif()
# -------------------------------------------------------


# -------------------------------------------------------
# Our Framework project
# -------------------------------------------------------
project(TobasaFramework VERSION 1.0
   DESCRIPTION "Modular C++ libraries for building HTTP services, SQL-backed applications, and laboratory instrument integrations")


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


if(TOBASA_BUILD_WITH_VCPKG AND NOT MSVC)
   message(FATAL_ERROR "VCPKG only valid for building with MSVC")
endif()

include(cmake/utils.cmake)

# Features check
include(cmake/tobasa_check_features.cmake)
tobasa_check_float_to_chars()
tobasa_check_std_chrono_parse()
configure_file(
   "${CMAKE_CURRENT_SOURCE_DIR}/cmake/tobasa_config.h.in"
   "${CMAKE_CURRENT_BINARY_DIR}/include/tobasa/tobasa_config.h")


# Helper functions
include(cmake/tobasa_prepare.cmake NO_POLICY_SCOPE)
include(cmake/tobasa_stage_shared_runtime.cmake)
if(TOBASA_BUILD_TESTS)
   include(cmake/tobasa_testing.cmake)
endif()


set(TOBASA_FRAMEWORK_ROOTDIR     ${CMAKE_CURRENT_SOURCE_DIR} )
set(TOBASA_FRAMEWORK_BINARY_DIR  ${CMAKE_BINARY_DIR} )
# external package dependencies location (asio,date,fmt ...)
set(TOBASA_EXTERNAL_DEPS_DIR     ${TOBASA_FRAMEWORK_ROOTDIR}/src/ext )


# Prebuilt libraries
if(MSVC AND NOT TOBASA_BUILD_WITH_VCPKG)

   # When these PREBUILT lib OFF, we build from source
   # ------------------------------------------------------
   set(TOBASA_USE_PREBUILT_ZLIB         OFF CACHE BOOL "")
   set(TOBASA_USE_PREBUILT_MINIZIP      OFF CACHE BOOL "")
   set(TOBASA_USE_PREBUILT_SQLITE3      OFF CACHE BOOL "")
   set(TOBASA_HTTP_USE_PREBUILT_NGHTTP2 OFF CACHE BOOL "")
   set(TOBASA_SQL_USE_PREBUILT_MARIADB  OFF CACHE BOOL "")

   set(TOBASA_SQL_USE_PREBUILT_LIBPQ    ON  CACHE BOOL "")   # we don't fetch and build libpq
   # ------------------------------------------------------

   set(TOBASA_ZLIB_DIR      ${TOBASA_ZLIB_DIR_DEF} )
   set(TOBASA_MINIZIP_DIR   ${TOBASA_MINIZIP_DIR_DEF} )
   set(TOBASA_SQLITE3_DIR   ${TOBASA_SQLITE3_DIR_DEF})
   set(TOBASA_OPENSSL_DIR   ${TOBASA_OPENSSL_DIR_DEF} )

   set(TOBASA_LIBPQ_DIR     ${TOBASA_LIBPQ_DIR_DEF} )
   set(TOBASA_MARIADB_DIR   ${TOBASA_MARIADB_DIR_DEF})
   set(TOBASA_PROTOBUF_DIR  ${TOBASA_PROTOBUF_DIR_DEF} ) # chat
   set(TOBASA_HTTP2_DIR     ${TOBASA_HTTP2_DIR_DEF} )

elseif(NOT WIN32)
   set(TOBASA_PROTOBUF_DIR  ${TOBASA_PROTOBUF_DIR_DEF} )
   set(TOBASA_HTTP2_DIR     ${TOBASA_HTTP2_DIR_DEF} )
endif()


# Only do these if this is the main project,
# and not if it is included through add_subdirectory
set(TOBASA_FRAMEWORK_MASTER_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
   SET(TOBASA_FRAMEWORK_MASTER_PROJECT ON)

   # Docs only available if this is the main app
   find_package(Doxygen)
   if(Doxygen_FOUND)
      add_subdirectory(docs)
   else()
      message(STATUS "tobasa Doxygen not found, skipping documentation build")
   endif()
endif()


# Zlib library
include(cmake/tobasa_zlib.cmake)
tobasa_configure_zlib()


# SQLite3 library
include(cmake/tobasa_sqlite3.cmake)
tobasa_configure_sqlite3()


# Disable selected MSVC C++ warnings.
add_compile_options(
   "$<$<COMPILE_LANGUAGE:CXX>:/wd4251>"  # DLL interface warning
   "$<$<COMPILE_LANGUAGE:CXX>:/wd4275>"  # Non-DLL base class warning
   "$<$<COMPILE_LANGUAGE:CXX>:/wd4566>"  # Character encoding warning
)


# -------------------------------------------------------
# Projects to build
# -------------------------------------------------------

if(TOBASA_BUILD_MONOLITHIC_SHARED_LIB)
   add_subdirectory(src/monolithic)
endif()

# Tobasa core libs
add_subdirectory(src/tobasa)
add_subdirectory(src/tobasasql)
add_subdirectory(src/tobasahttp)
add_subdirectory(src/tobasaweb)

if(TOBASA_BUILD_LIS_ENGINE)
   add_subdirectory(src/tobasalis)
endif()

add_subdirectory(src/app_server)

if(TOBASA_BUILD_SHARED_LIB)
   add_subdirectory(src/plugins)
endif()

add_subdirectory(src/samples)


# -------------------------------------------------------
if(TOBASA_BUILD_WITH_VCPKG)
   message(STATUS "tobasa Using VCPKG as CMake toolchain")
endif()
message(STATUS "tobasa Build shared library     : ${TOBASA_BUILD_SHARED_LIB}" )
message(STATUS "tobasa Build monolithic library : ${TOBASA_BUILD_MONOLITHIC_SHARED_LIB}" )
message(STATUS "tobasa External dependencies    : ${TOBASA_EXTERNAL_DEPS_DIR}" )