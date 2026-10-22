cmake_minimum_required(VERSION 3.12-3.18)
cmake_policy(SET CMP0091 NEW)
if(TOBASA_BUILD_MONOLITHIC_SHARED_LIB)
   cmake_policy(SET CMP0079 NEW)
endif()

# The project
project(tobasaweb VERSION 1.0 LANGUAGES CXX)

if(NOT DEFINED TOBASA_EXTERNAL_DEPS_DIR)
   set(TOBASA_EXTERNAL_DEPS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ext")
endif()

option(TOBASA_FIND_DEPS "Use `find_package()` for including Tobasa Web dependencies." OFF)

if(TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG)
   find_package(spdlog REQUIRED)
else()
   set(SPDLOG_INCLUDE_DIR ${TOBASA_EXTERNAL_DEPS_DIR}/spdlog/include)
   message(STATUS "tobasaweb spdlog include dir: ${SPDLOG_INCLUDE_DIR}")
endif()


# Use bundled nlohmann and jwt-cpp package
set(JWTCPP_INCLUDE_DIR   ${TOBASA_EXTERNAL_DEPS_DIR}/jwt-cpp/include)
message(STATUS "tobasaweb jwt-cpp include dir: ${JWTCPP_INCLUDE_DIR}")

# The library
if(TOBASA_BUILD_MONOLITHIC_SHARED_LIB)
   set(TOBASA_MODULE_TARGET tobasa_monolithic)
   target_compile_definitions(${TOBASA_MODULE_TARGET} PRIVATE TBS_WEB_SHARED)
elseif(TOBASA_BUILD_SHARED_LIB)
   set(TOBASA_MODULE_TARGET tobasaweb)
   add_library(tobasaweb SHARED)
   target_compile_definitions(${TOBASA_MODULE_TARGET} PRIVATE TBS_WEB_SHARED)
else()
   set(TOBASA_MODULE_TARGET tobasaweb)
   add_library(tobasaweb STATIC)
   target_compile_definitions(${TOBASA_MODULE_TARGET} PUBLIC TBS_WEB_STATIC)
endif()

include(${PROJECT_SOURCE_DIR}/cmake/sources.cmake)

target_sources(${TOBASA_MODULE_TARGET}
   PRIVATE ${MY_SOURCES_LIST}
   PRIVATE ${MY_HEADERS_LIST} )

if(NOT TOBASA_BUILD_MONOLITHIC_SHARED_LIB)
   target_link_libraries(${TOBASA_MODULE_TARGET}
      PRIVATE tobasa
      PRIVATE tobasasql
      PRIVATE tobasahttp)
endif()

target_include_directories(${TOBASA_MODULE_TARGET}
   PUBLIC ${JWTCPP_INCLUDE_DIR}
   PUBLIC ${PROJECT_SOURCE_DIR}/include
   PUBLIC ${PROJECT_SOURCE_DIR}/src )

if(TOBASA_BUILD_TESTS)
   add_subdirectory(tests)
endif()

if(TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG)
   target_link_libraries(${TOBASA_MODULE_TARGET} PUBLIC spdlog::spdlog)
else()
   message(STATUS "${PROJECT_NAME} use bundled spdlog")
   target_include_directories(${TOBASA_MODULE_TARGET} PUBLIC ${SPDLOG_INCLUDE_DIR})

   set(HAS_STD_TEXT_FORMATTING FALSE) # __cpp_lib_format (Text formatting)
   if(MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.30.30705)
      set(HAS_STD_TEXT_FORMATTING TRUE)
   elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)
      set(HAS_STD_TEXT_FORMATTING TRUE)
   endif()

   if(HAS_STD_TEXT_FORMATTING)
      target_compile_definitions(${TOBASA_MODULE_TARGET} PUBLIC SPDLOG_USE_STD_FORMAT )
   else()
      target_compile_definitions(${TOBASA_MODULE_TARGET} PUBLIC SPDLOG_FMT_EXTERNAL )
   endif()
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
   PREFIX "Header Files"
   FILES ${MY_HEADERS_LIST} )

# Show filtered sources in IDE
source_group(
   TREE ${PROJECT_SOURCE_DIR}/src
   PREFIX "Source Files"
   FILES ${MY_SOURCES_LIST} )
