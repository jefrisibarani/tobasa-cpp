cmake_minimum_required(VERSION 3.12-3.18)
cmake_policy(SET CMP0091 NEW)

# The project
project(tobasalis VERSION 1.0 LANGUAGES CXX)

if(NOT DEFINED TOBASA_EXTERNAL_DEPS_DIR)
   set(TOBASA_EXTERNAL_DEPS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ext")
endif()

option(TOBASA_FIND_DEPS "Use `find_package()` for including Tobasa LIS dependencies." OFF)

# Use bundled asio package
set(ASIO_INCLUDE_DIR ${TOBASA_EXTERNAL_DEPS_DIR}/asio/include)
message(STATUS "tobasalis Asio include folder: ${ASIO_INCLUDE_DIR}")

# The library
add_library(${PROJECT_NAME})
include(${PROJECT_SOURCE_DIR}/cmake/sources.cmake)

if(MSVC)
   set_property(TARGET ${PROJECT_NAME} PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

target_sources(${PROJECT_NAME}
   PRIVATE ${MY_SOURCES_LIST}
   PRIVATE ${MY_HEADERS_LIST})

target_link_libraries(${PROJECT_NAME}
   PRIVATE tobasa )

target_include_directories(${PROJECT_NAME}
   PUBLIC ${ASIO_INCLUDE_DIR}
   PUBLIC ${PROJECT_SOURCE_DIR}/include )

target_compile_definitions(${PROJECT_NAME} PUBLIC ASIO_STANDALONE )

if(MSVC)
   target_compile_definitions(${PROJECT_NAME}
      PUBLIC _WIN32_WINNT=0x0601 _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING NOMINMAX)
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
   TREE ${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}
   PREFIX "Header Files"
   FILES ${MY_HEADERS_LIST} )

# Show filtered sources in IDE
source_group(
   TREE ${PROJECT_SOURCE_DIR}/src
   PREFIX "Source Files"
   FILES ${MY_SOURCES_LIST} )