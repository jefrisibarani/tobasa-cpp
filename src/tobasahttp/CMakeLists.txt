cmake_minimum_required(VERSION 3.12-3.18)
cmake_policy(SET CMP0091 NEW)

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
   set(ZLIB_LIBS_HTTP optimized ${TOBASA_ZLIB_DIR}/lib/zlib_o.lib debug ${TOBASA_ZLIB_DIR}/lib/zlib_d.lib )
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
   if(NOT DEFINED TOBASA_HTTP2_DIR)
      if(MSVC)
         set(TOBASA_HTTP2_DIR ${TOBASA_HTTP2_DIR_DEF} )
      else()
         set(TOBASA_HTTP2_DIR ${TOBASA_HTTP2_DIR_DEF} )
      endif()
   endif()
endif()

## no more dependecies dir settings here
# The library
add_library(${PROJECT_NAME})
include(${PROJECT_SOURCE_DIR}/cmake/sources.cmake)

if(MSVC)
   set_property(TARGET ${PROJECT_NAME} PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

target_sources( ${PROJECT_NAME}
   PRIVATE ${MY_SOURCES_LIST}
   PRIVATE ${MY_HEADERS_LIST} )

target_link_libraries( ${PROJECT_NAME}
   PRIVATE tobasa
   PRIVATE ${OPENSSL_LIBRARIES} 
   PRIVATE ${ZLIB_LIBS_HTTP} )

if(MSVC)
   target_link_libraries( ${PROJECT_NAME} PUBLIC crypt32.lib )
endif()

if(TOBASA_HTTP_USE_HTTP2)
   target_include_directories( ${PROJECT_NAME} PUBLIC ${TOBASA_HTTP2_DIR}/include )
   if (MSVC)
      target_link_libraries( ${PROJECT_NAME} PUBLIC ${TOBASA_HTTP2_DIR}/lib/nghttp2.lib )
   else()
      target_link_libraries( ${PROJECT_NAME} PUBLIC ${TOBASA_HTTP2_DIR}/lib/libnghttp2.a )
   endif()
endif()

target_include_directories( ${PROJECT_NAME}
   PUBLIC ${ZLIB_INC_DIR_HTTP}
   PUBLIC ${OPENSSL_INCLUDE_DIR}
   PUBLIC ${ASIO_INCLUDE_DIR}
   PUBLIC ${PROJECT_SOURCE_DIR}/include
   PUBLIC ${PROJECT_SOURCE_DIR}/src 
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../tobasa/include" )

target_compile_definitions(${PROJECT_NAME} PUBLIC ASIO_STANDALONE )

if (TOBASA_HTTP_USE_HTTP2)
   target_compile_definitions( ${PROJECT_NAME} 
      PUBLIC TOBASA_HTTP_USE_HTTP2
      PUBLIC TOBASA_HTTP2_WRITE_RESPONSE_NO_COPY_DATA
      PUBLIC NGHTTP2_NO_SSIZE_T)
endif()

if (MSVC)
   target_compile_definitions(${PROJECT_NAME} PUBLIC _WIN32_WINNT=0x0601 _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING NOMINMAX)
endif()

if(CMAKE_CXX20_COMPILE_FEATURES)
   message(STATUS "${PROJECT_NAME} use cxx_std_20")
   target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
else()
   message(STATUS "${PROJECT_NAME} use cxx_std_17")
   target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
endif()

if(MSVC)
   target_compile_options(${PROJECT_NAME} PRIVATE /bigobj /FS )
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
