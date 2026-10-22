cmake_minimum_required(VERSION 3.12-3.18)
cmake_policy(SET CMP0091 NEW)
if(TOBASA_BUILD_MONOLITHIC_SHARED_LIB)
   cmake_policy(SET CMP0079 NEW)
endif()

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
if(TOBASA_BUILD_MONOLITHIC_SHARED_LIB)
   set(TOBASA_MODULE_TARGET tobasa_monolithic)
   target_compile_definitions(${TOBASA_MODULE_TARGET} PRIVATE TBS_LIS_SHARED)
elseif(TOBASA_BUILD_SHARED_LIB)
   set(TOBASA_MODULE_TARGET ${PROJECT_NAME})
   add_library(${PROJECT_NAME} SHARED)
   target_compile_definitions(${TOBASA_MODULE_TARGET} PRIVATE TBS_LIS_SHARED)
else()
   set(TOBASA_MODULE_TARGET ${PROJECT_NAME})
   add_library(${PROJECT_NAME} STATIC)
   target_compile_definitions(${TOBASA_MODULE_TARGET} PUBLIC TBS_LIS_STATIC)
endif()

include(${PROJECT_SOURCE_DIR}/cmake/sources.cmake)

target_sources(${TOBASA_MODULE_TARGET}
   PRIVATE ${MY_SOURCES_LIST}
   PRIVATE ${MY_HEADERS_LIST})

if(NOT TOBASA_BUILD_MONOLITHIC_SHARED_LIB)
   target_link_libraries(${TOBASA_MODULE_TARGET} PUBLIC tobasa)
endif()

target_include_directories(${TOBASA_MODULE_TARGET}
   PUBLIC ${ASIO_INCLUDE_DIR}
   PUBLIC ${PROJECT_SOURCE_DIR}/include )

if(TOBASA_BUILD_TESTS)
   add_subdirectory(tests)
endif()

target_compile_definitions(${TOBASA_MODULE_TARGET}
   PUBLIC ASIO_STANDALONE
   PUBLIC ASIO_USE_TS_EXECUTOR_AS_DEFAULT )

if(MSVC)
   target_compile_definitions(${TOBASA_MODULE_TARGET}
      PUBLIC _WIN32_WINNT=0x0601 _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING NOMINMAX)
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
   TREE ${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}
   PREFIX "Header Files"
   FILES ${MY_HEADERS_LIST} )

# Show filtered sources in IDE
source_group(
   TREE ${PROJECT_SOURCE_DIR}/src
   PREFIX "Source Files"
   FILES ${MY_SOURCES_LIST} )