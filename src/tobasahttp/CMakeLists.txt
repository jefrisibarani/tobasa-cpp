cmake_minimum_required(VERSION 3.12-3.18)
cmake_policy(SET CMP0091 NEW)
if(TOBASA_BUILD_MONOLITHIC_SHARED_LIB)
   cmake_policy(SET CMP0079 NEW)
endif()

# The project
project(tobasahttp VERSION 1.0 LANGUAGES CXX)

option(TOBASA_FIND_DEPS            "Use `find_package()` for including Tobasa HTTP dependencies." OFF)
option(TOBASA_HTTP_USE_HTTP2       "Enable HTTP/2 support"              ON)

if(NOT DEFINED TOBASA_EXTERNAL_DEPS_DIR)
   set(TOBASA_EXTERNAL_DEPS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ext")
endif()

if(MSVC AND NOT TOBASA_BUILD_WITH_VCPKG AND NOT DEFINED TOBASA_OPENSSL_DIR)
   set(TOBASA_OPENSSL_DIR ${TOBASA_OPENSSL_DIR_DEF} )
endif()

if(MSVC AND DEFINED TOBASA_OPENSSL_DIR)
   set(ENV{OPENSSL_ROOT_DIR} ${TOBASA_OPENSSL_DIR})
endif()

# Use bundled asio, nlohmann and jwt-cpp package
set(ASIO_INCLUDE_DIR     ${TOBASA_EXTERNAL_DEPS_DIR}/asio/include)
message(STATUS "tobasahttp Asio include dir: ${ASIO_INCLUDE_DIR}")

# Alway use find_package for OpenSSL
# if we set OPENSSL_ROOT_DIR environment variable, find_package will find in that dir
find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
   message(STATUS "tobasahttp OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
   message(STATUS "tobasahttp OpenSSL libraries: ${OPENSSL_LIBRARIES}")
endif(OPENSSL_FOUND)


if(TOBASA_USE_PREBUILT_ZLIB) # only for windows build
   set(ZLIB_LIBS_HTTP optimized ${TOBASA_ZLIB_DIR}/lib/zlib.lib debug ${TOBASA_ZLIB_DIR}/lib/zlibd.lib )
   set(ZLIB_INC_DIR_HTTP ${TOBASA_ZLIB_DIR}/include)
elseif(TOBASA_USE_BUNDLED_ZLIB)
   set(ZLIB_LIBS_HTTP ZLIB::ZLIBSTATIC )
   set(ZLIB_INC_DIR_HTTP "")
else()
   if(NOT ZLIB_FOUND)
      find_package(ZLIB REQUIRED)
   endif()
   set(ZLIB_LIBS_HTTP ${ZLIB_LIBRARIES})
   set(ZLIB_INC_DIR_HTTP ${ZLIB_INCLUDE_DIR})
endif()

if(TOBASA_HTTP_USE_HTTP2)
   if(TOBASA_BUILD_WITH_VCPKG)
      set(TOBASA_VCPKG_NGHTTP2_ROOT
         "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
      set(TOBASA_HTTP2_INCLUDE_DIR "${TOBASA_VCPKG_NGHTTP2_ROOT}/include")
      set(TOBASA_HTTP2_RELEASE_LIBRARY "${TOBASA_VCPKG_NGHTTP2_ROOT}/lib/nghttp2.lib")
      set(TOBASA_HTTP2_DEBUG_LIBRARY "${TOBASA_VCPKG_NGHTTP2_ROOT}/debug/lib/nghttp2.lib")

      if(NOT EXISTS "${TOBASA_HTTP2_INCLUDE_DIR}/nghttp2/nghttp2.h"
         OR NOT EXISTS "${TOBASA_HTTP2_RELEASE_LIBRARY}"
         OR NOT EXISTS "${TOBASA_HTTP2_DEBUG_LIBRARY}")
         message(FATAL_ERROR
            "nghttp2 was not found in Vcpkg triplet '${VCPKG_TARGET_TRIPLET}' "
            "under '${TOBASA_VCPKG_NGHTTP2_ROOT}'. Install the nghttp2 port for this triplet.")
      endif()

      set(TOBASA_HTTP2_TARGET
         optimized ${TOBASA_HTTP2_RELEASE_LIBRARY}
         debug ${TOBASA_HTTP2_DEBUG_LIBRARY})
   elseif(UNIX OR TOBASA_FIND_DEPS)
      find_package(PkgConfig REQUIRED)
      pkg_check_modules(NGHTTP2 REQUIRED IMPORTED_TARGET libnghttp2)
      set(TOBASA_HTTP2_TARGET PkgConfig::NGHTTP2)
   else()
      if(NOT DEFINED TOBASA_HTTP2_DIR)
         set(TOBASA_HTTP2_DIR ${TOBASA_HTTP2_DIR_DEF})
      endif()
      set(TOBASA_HTTP2_TARGET
         optimized ${TOBASA_HTTP2_DIR}/lib/nghttp2.lib
         debug ${TOBASA_HTTP2_DIR}/debug/lib/nghttp2.lib)
   endif()
endif()

## no more dependecies dir settings here
# The library
if(TOBASA_BUILD_MONOLITHIC_SHARED_LIB)
   set(TOBASA_MODULE_TARGET tobasa_monolithic)
   target_compile_definitions(${TOBASA_MODULE_TARGET} PRIVATE TBS_HTTP_SHARED)
elseif(TOBASA_BUILD_SHARED_LIB)
   set(TOBASA_MODULE_TARGET ${PROJECT_NAME})
   add_library(${PROJECT_NAME} SHARED)
   target_compile_definitions(${TOBASA_MODULE_TARGET} PRIVATE TBS_HTTP_SHARED)
else()
   set(TOBASA_MODULE_TARGET ${PROJECT_NAME})
   add_library(${PROJECT_NAME} STATIC)
   target_compile_definitions(${TOBASA_MODULE_TARGET} PUBLIC TBS_HTTP_STATIC)
endif()

include(${PROJECT_SOURCE_DIR}/cmake/sources.cmake)

target_sources(${TOBASA_MODULE_TARGET}
   PRIVATE ${MY_SOURCES_LIST}
   PRIVATE ${MY_HEADERS_LIST} )

if(NOT TOBASA_BUILD_MONOLITHIC_SHARED_LIB)
   target_link_libraries(${TOBASA_MODULE_TARGET} PRIVATE tobasa)
endif()

target_link_libraries(${TOBASA_MODULE_TARGET}
   PUBLIC ${OPENSSL_LIBRARIES}
   PRIVATE ${ZLIB_LIBS_HTTP} )

if(MSVC)
   target_link_libraries(${TOBASA_MODULE_TARGET} PUBLIC crypt32.lib)
endif()

if(TOBASA_HTTP_USE_HTTP2)
   target_link_libraries(${TOBASA_MODULE_TARGET} PUBLIC ${TOBASA_HTTP2_TARGET})
   if(DEFINED TOBASA_HTTP2_INCLUDE_DIR)
      target_include_directories(${TOBASA_MODULE_TARGET} PUBLIC ${TOBASA_HTTP2_INCLUDE_DIR})
   elseif(NOT UNIX)
      target_include_directories(${TOBASA_MODULE_TARGET} PUBLIC ${TOBASA_HTTP2_DIR}/include)
   endif()
endif()

target_include_directories(${TOBASA_MODULE_TARGET}
   PUBLIC ${ZLIB_INC_DIR_HTTP}
   PUBLIC ${OPENSSL_INCLUDE_DIR}
   PUBLIC ${ASIO_INCLUDE_DIR}
   PUBLIC ${PROJECT_SOURCE_DIR}/include
   PUBLIC ${PROJECT_SOURCE_DIR}/src 
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../tobasa/include" )

target_compile_definitions(${TOBASA_MODULE_TARGET}
   PUBLIC ASIO_STANDALONE
   PUBLIC ASIO_USE_TS_EXECUTOR_AS_DEFAULT )

if (TOBASA_HTTP_USE_HTTP2)
   target_compile_definitions(${TOBASA_MODULE_TARGET}
      PUBLIC TOBASA_HTTP_USE_HTTP2
      PUBLIC TOBASA_HTTP2_WRITE_RESPONSE_NO_COPY_DATA
      PUBLIC NGHTTP2_NO_SSIZE_T)
endif()

if (MSVC)
   target_compile_definitions(${TOBASA_MODULE_TARGET} PUBLIC _WIN32_WINNT=0x0601 _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING NOMINMAX)
endif()

if(CMAKE_CXX20_COMPILE_FEATURES)
   message(STATUS "${PROJECT_NAME} use cxx_std_20")
   target_compile_features(${TOBASA_MODULE_TARGET} PUBLIC cxx_std_20)
else()
   message(STATUS "${PROJECT_NAME} use cxx_std_17")
   target_compile_features(${TOBASA_MODULE_TARGET} PUBLIC cxx_std_17)
endif()

if(MSVC)
   target_compile_options(${TOBASA_MODULE_TARGET} PRIVATE /bigobj /FS )
endif()

# Show filtered headers in IDE
source_group(
   TREE ${PROJECT_SOURCE_DIR}/include
   PREFIX "Header Files tobasahttp"
   FILES ${MY_HEADERS_LIST} )

# Show filtered sources in IDE
source_group(
   TREE ${PROJECT_SOURCE_DIR}/src
   PREFIX "Source Files"
   FILES ${MY_SOURCES_LIST} )

if(TOBASA_BUILD_TESTS)
   add_subdirectory(tests)
endif()
