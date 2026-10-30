cmake_minimum_required(VERSION 3.14-3.18)
cmake_policy(SET CMP0091 NEW)
if(TOBASA_BUILD_MONOLITHIC_SHARED_LIB)
   cmake_policy(SET CMP0079 NEW)
endif()

project(tobasa VERSION 1.0 LANGUAGES CXX)

if(NOT DEFINED TOBASA_EXTERNAL_DEPS_DIR)
   set(TOBASA_EXTERNAL_DEPS_DIR "../ext")
endif()

option(TOBASA_FIND_DEPS "Use `find_package()` for including Tobasa dependencies." OFF)
option(TOBASA_BUILD_IN_MEMORY_TZDB   "Whether to use compiled timezone data" OFF )


# -------------------------------------------------------
if(NOT CMAKE_CXX17_COMPILE_FEATURES)
   message(FATAL_ERROR "Compiler not supported, use compiler with cxx_std_17")
endif()

if(NOT TOBASA_STD_FORMAT_AND_DATE_CHECKED)
   if(MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.30.30705)
      set(TOBASA_USE_STD_TEXT_FORMATTING TRUE CACHE INTERNAL "" ) # __cpp_lib_format >= 201907L (Text formatting)
      set(TOBASA_USE_STD_CALENDAR_TZONE  TRUE CACHE INTERNAL "" ) # __cpp_lib_chrono >= 201803L (Calendar and Time Zone library) 
   elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 14.0)
      set(TOBASA_USE_STD_TEXT_FORMATTING TRUE CACHE INTERNAL "" )
      set(TOBASA_USE_STD_CALENDAR_TZONE  TRUE CACHE INTERNAL "" )
   else()
      set(TOBASA_USE_STD_CALENDAR_TZONE FALSE CACHE INTERNAL "" )
      set(TOBASA_USE_STD_TEXT_FORMATTING FALSE CACHE INTERNAL "" )
   endif()
endif()

if(TOBASA_BUILD_WITH_VCPKG)
   message(STATUS "tobasa VCPKG Target triplet ${VCPKG_TARGET_TRIPLET}")
   message(STATUS "tobasa VCPKG Overlay ports ${VCPKG_OVERLAY_PORTS}")
   message(STATUS "tobasa CMake toolchain ${CMAKE_TOOLCHAIN_FILE}")
endif() 


# -------------------------------------------------------
# Formatting library
# -------------------------------------------------------
if(NOT TOBASA_USE_STD_TEXT_FORMATTING)
   if(TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG)
      find_package(fmt CONFIG REQUIRED) 
   else()
      message(STATUS "tobasa Using bundled format library")
      add_subdirectory(${TOBASA_EXTERNAL_DEPS_DIR}/fmt ${CMAKE_BINARY_DIR}/_exts/fmt EXCLUDE_FROM_ALL)
   endif()
else()
   message(STATUS "tobasa Using std::fmt formatting library")
endif()


# -------------------------------------------------------
# Date and timezone library
# -------------------------------------------------------
if(NOT TOBASA_USE_STD_CALENDAR_TZONE)
   include(cmake/tobasa_date_timezone.cmake)
   tobasa_configure_date_and_timezone_lib()
else()
   message(STATUS "tobasa Using std::date date library")
endif()


# -------------------------------------------------------
# spdlog
# -------------------------------------------------------
if(TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG)
   find_package(spdlog REQUIRED)
else()
   set(TOBASA_SPDLOG_INCLUDE_DIRS ${TOBASA_EXTERNAL_DEPS_DIR}/spdlog/include)
   message(STATUS "tobasa spdlog include dir: ${TOBASA_SPDLOG_INCLUDE_DIRS}")
endif()


# -------------------------------------------------------
# The library
# -------------------------------------------------------
if(TOBASA_BUILD_MONOLITHIC_SHARED_LIB)
   set(TOBASA_MODULE_TARGET tobasa_monolithic)
   target_compile_definitions(${TOBASA_MODULE_TARGET} PRIVATE TBS_SHARED)
elseif(TOBASA_BUILD_SHARED_LIB)
   set(TOBASA_MODULE_TARGET tobasa)
   add_library(tobasa SHARED)
   target_compile_definitions(${TOBASA_MODULE_TARGET} PRIVATE TBS_SHARED)
else()
   set(TOBASA_MODULE_TARGET tobasa)
   add_library(tobasa STATIC)
   target_compile_definitions(${TOBASA_MODULE_TARGET} PUBLIC TBS_STATIC)
endif()

include(${PROJECT_SOURCE_DIR}/cmake/sources.cmake)

function(configure_tobasa_target target)
   target_sources(${target}
      PRIVATE ${MY_SOURCES_LIST}
      PRIVATE ${MY_HEADERS_LIST})

   target_include_directories(${target}
      PUBLIC ${TOBASA_EXTERNAL_DEPS_DIR}/nlohmann/include
      PUBLIC ${TOBASA_EXTERNAL_DEPS_DIR}/utf8cpp/include
      PUBLIC ${PROJECT_SOURCE_DIR}/include)

   if(NOT TOBASA_USE_STD_TEXT_FORMATTING)
      if(TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG)
         target_link_libraries(${target} PUBLIC fmt::fmt)
      else()
         message(STATUS "tobasa Using bundled Formatting fmt library")
         target_link_libraries(${target} PUBLIC fmt::fmt)
      endif()
   endif()

   if(NOT TOBASA_USE_STD_CALENDAR_TZONE)
      if(NOT TOBASA_USE_BUNDLED_TZ_LIB)
         target_link_libraries(${target} PUBLIC date::date date::date-tz)
      else()
         target_include_directories(${target} PUBLIC ${TOBASA_EXTERNAL_DEPS_DIR}/date/include)
         target_link_libraries(${target} PUBLIC timezone)

         if(WIN32)
            if(TOBASA_BUILD_IN_MEMORY_TZDB)
               target_compile_definitions(${target} PUBLIC TOBASA_DATE_USE_IN_MEMORY_TZDB)
            endif()
         else()
            if(TOBASA_BUILD_IN_MEMORY_TZDB)
               target_compile_definitions(${target} PUBLIC TOBASA_DATE_USE_IN_MEMORY_TZDB)
            else()
               target_compile_definitions(${target} PUBLIC TOBASA_DATE_USE_OS_TZDB)
            endif()
         endif()

      endif()
   endif()

   if(TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG)
      target_link_libraries(${target} PRIVATE spdlog::spdlog)
   else()
      message(STATUS "${target} use bundled spdlog")
      target_include_directories(${target} PRIVATE ${TOBASA_SPDLOG_INCLUDE_DIRS})

      if(TOBASA_USE_STD_TEXT_FORMATTING)
         target_compile_definitions(${target} PRIVATE SPDLOG_USE_STD_FORMAT )
      else()
         target_compile_definitions(${target} PRIVATE SPDLOG_FMT_EXTERNAL )
      endif()
   endif()

   if(MSVC)
      target_compile_definitions(${target} PUBLIC _CRT_SECURE_NO_WARNINGS _UNICODE NOMINMAX)
      target_compile_options(${target} PRIVATE /bigobj /FS)
      target_link_libraries(${target} PUBLIC shlwapi)
   endif()

   target_link_libraries(${target} PRIVATE ${CMAKE_DL_LIBS})

   if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
      target_compile_options(${target} PRIVATE -fpic -Wall)
      target_compile_options(${target} PUBLIC -pthread )
      target_link_libraries(${target} PUBLIC pthread)
   endif()

   if("cxx_std_20" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
      message(STATUS "tobasa C++20 compile features are supported")
      target_compile_features(${target} PUBLIC cxx_std_20)
   else()
      message(STATUS "tobasa C++17 compile features are supported")
      target_compile_features(${target} PUBLIC cxx_std_17)
   endif()

endfunction()

configure_tobasa_target(${TOBASA_MODULE_TARGET})


# Show filtered headers in IDE
source_group(
   TREE ${PROJECT_SOURCE_DIR}/include/tobasa
   PREFIX "Header Files"
   FILES ${MY_HEADERS_LIST} )

# Show filtered sources in IDE
source_group(
   TREE ${PROJECT_SOURCE_DIR}/src
   PREFIX "Source Files"
   FILES ${MY_SOURCES_LIST} )

if(TOBASA_BUILD_TESTS)
   add_subdirectory(tests)
endif()