cmake_minimum_required(VERSION 3.12-3.18)
include(CMakeDependentOption)

# Global/default MSVC Runtime is MT
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY   "MultiThreaded$<$<CONFIG:Debug>:Debug>" )

# -------------------------------------------------------

# Valid only with MSVC Compiler
option(TOBASA_BUILD_WITH_VCPKG   "Build Tobasa with VCPKG libs"       OFF )

# Database Drivers
option(TOBASA_SQL_USE_PGSQL      "Enable PostgreSQL driver"           ON )
option(TOBASA_SQL_USE_ADODB      "Enable ADO driver"                  ON )
option(TOBASA_SQL_USE_ODBC       "Enable ODBC driver"                 ON )
option(TOBASA_SQL_USE_SQLITE     "Enable SQLite driver"               ON )
option(TOBASA_SQL_USE_MYSQL      "Enable MariaDB driver"              ON )

option(TOBASA_BUILD_LIS_ENGINE   "Build LIS Engine"                   ON )

option(TOBASA_BUILD_TESTS        "Build Test Module"                  ON )
option(TOBASA_BUILD_TESTS_WSCHAT_USE_PROBUF 
                                 "Build Test Websocket with protobuf" ON )

option(TOBASA_HTTP_USE_HTTP2     "Enable HTTP/2 support"              ON )

CMAKE_DEPENDENT_OPTION(TOBASA_BUILD_TESTS_WSCHAT_USE_PROBUF 
                       "Build Test Websocket with protobuf"   ON "TOBASA_BUILD_TESTS" ON )
CMAKE_DEPENDENT_OPTION(TOBASA_SQL_USE_MARIADB_DLL 
                       "Use libmariadb shared (.dll) library" OFF "TOBASA_SQL_USE_MYSQL" OFF )

# -------------------------------------------------------
option(TOBASA_BUILD_IN_MEMORY_RESOURCES "Whether to use compiled resources"     ON )
option(TOBASA_BUILD_IN_MEMORY_TZDB      "Whether to use compiled timezone data" ON )


set(TOBASA_PACKAGE_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/_output CACHE STRING "Location of package directory" )
set(TOBASA_CREATE_PACKAGE_OUTPUT_DIR ON CACHE BOOL "Create a package directory bundle" )


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

# Use prebuilt zlib library (on windows only)
set(TOBASA_USE_PREBUILT_ZLIB ON)

# -------------------------------------------------------
# Our Framework project
# -------------------------------------------------------
project(TobasaFramework VERSION 1.0
   DESCRIPTION "Provide SQL, LIS, DICOM, HTTP and WEB App")

if(TOBASA_BUILD_WITH_VCPKG AND MSVC)
   message(STATUS "Using VCPKG as CMake toolchain")
elseif(TOBASA_BUILD_WITH_VCPKG)
   message(FATAL_ERROR "VCPKG only valid for building with MSVC")
endif()

include(cmake/tobasa_prepare.cmake NO_POLICY_SCOPE)

set(TOBASA_FRAMEWORK_ROOTDIR     ${CMAKE_CURRENT_SOURCE_DIR} )
set(TOBASA_FRAMEWORK_BINARY_DIR  ${CMAKE_BINARY_DIR} )
# external package dependencies location (asio,date,fmt ...)
set(TOBASA_EXTERNAL_DEPS_DIR     ${TOBASA_FRAMEWORK_ROOTDIR}/src/ext )
message(STATUS "Tobasa: External dependencies folder => ${TOBASA_EXTERNAL_DEPS_DIR}" )


if(MSVC AND TOBASA_BUILD_WITH_VCPKG)
   set(TOBASA_USE_PREBUILT_ZLIB OFF)
elseif(MSVC AND NOT TOBASA_BUILD_WITH_VCPKG)
   if(NOT TOBASA_USE_PREBUILT_ZLIB)
      set(TOBASA_USE_BUNDLED_ZLIB ON CACHE BOOL "Use bundled zlib library")
   endif()

   # tobasasql(mysql), tobasahttp
   set(TOBASA_ZLIB_DIR           ${TOBASA_ZLIB_DIR_DEF} CACHE STRING "" )
   # tobasahttp
   set(TOBASA_OPENSSL_DIR        ${TOBASA_OPENSSL_DIR_DEF} CACHE STRING "")
   # tobasasql
   set(TOBASA_LIBPQ_DIR          ${TOBASA_LIBPQ_DIR_DEF} CACHE STRING "" )
   # chat module
   set(TOBASA_PROTOBUF_DIR       ${TOBASA_PROTOBUF_DIR_DEF} CACHE STRING "" )

   if(TOBASA_HTTP_USE_HTTP2)
      # HTTP/2 support
      set(TOBASA_HTTP2_DIR       ${TOBASA_HTTP2_DIR_DEF} CACHE STRING "" )
   endif()

elseif(NOT WIN32)
   set(TOBASA_USE_PREBUILT_ZLIB  OFF)
   set(TOBASA_USE_BUNDLED_ZLIB   OFF)

   set(TOBASA_PROTOBUF_DIR       ${TOBASA_PROTOBUF_DIR_DEF} CACHE STRING "" )

   if(TOBASA_HTTP_USE_HTTP2)
      set(TOBASA_HTTP2_DIR       ${TOBASA_HTTP2_DIR_DEF} CACHE STRING "" )
   endif()
endif()


# -------------------------------------------------------

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
      message(STATUS "Tobasa: Doxygen not found, skipping documentation build")
   endif()
endif()


# Zlib library
if(TOBASA_USE_BUNDLED_ZLIB)
   message(STATUS "Tobasa: using bundled zlib library")
   add_subdirectory(${TOBASA_EXTERNAL_DEPS_DIR}/zlib ${CMAKE_SOURCE_DIR}/build_ext/zlib EXCLUDE_FROM_ALL)
elseif(TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG)
   find_package(ZLIB REQUIRED)
endif()


# -------------------------------------------------------
# Add core modules to build
add_subdirectory(src/tobasa)
add_subdirectory(src/tobasasql)
add_subdirectory(src/tobasahttp)
add_subdirectory(src/tobasaweb)

if(TOBASA_BUILD_LIS_ENGINE)
   add_subdirectory(src/tobasalis)
endif()

add_subdirectory(src/app_server)
add_subdirectory(src/samples)