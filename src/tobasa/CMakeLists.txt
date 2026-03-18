cmake_minimum_required(VERSION 3.12-3.18)
cmake_policy(SET CMP0091 NEW)

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
      add_subdirectory(${TOBASA_EXTERNAL_DEPS_DIR}/fmt ${CMAKE_SOURCE_DIR}/build_ext/fmt EXCLUDE_FROM_ALL)
   endif()
else()
   message(STATUS "tobasa Using std::fmt formatting library")
endif()



# -------------------------------------------------------
# Date and timezone library
# -------------------------------------------------------
if(NOT TOBASA_USE_STD_CALENDAR_TZONE)
   if((TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG) AND NOT TOBASA_BUILD_IN_MEMORY_TZDB)
      find_package(date CONFIG REQUIRED)
      set(TOBASA_USE_BUNDLED_TZ_LIB FALSE CACHE INTERNAL "" )
   else()
      set(TOBASA_USE_BUNDLED_TZ_LIB TRUE CACHE INTERNAL "" )
      message(STATUS "tobasa Using bundled timezone library")

      if(TOBASA_BUILD_IN_MEMORY_TZDB)
         add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/resources/tzdata_assets.cpp"
            COMMAND generate_tzdata_assets_file ${TOBASA_FRAMEWORK_ROOTDIR}/res/tzdata tzdata ${CMAKE_CURRENT_BINARY_DIR}/resources/tzdata_assets.cpp getTzdataAsset tzdata
            DEPENDS generate_tzdata_assets_file
         )
      endif()

      add_library(timezone STATIC)
      target_sources( timezone PRIVATE ${TOBASA_EXTERNAL_DEPS_DIR}/date/src/tz.cpp )
      
      if(TOBASA_BUILD_IN_MEMORY_TZDB)
         target_sources( timezone PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/resources/tzdata_assets.cpp )
      endif()   

      if(MSVC)
         set_property(TARGET timezone PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
      endif()

      target_include_directories(timezone PRIVATE ${TOBASA_EXTERNAL_DEPS_DIR}/date/include )

      if(TOBASA_BUILD_IN_MEMORY_TZDB)
         target_include_directories(timezone 
            PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
            PRIVATE ${TOBASA_FRAMEWORK_ROOTDIR}/src/tobasa/include)
      endif()
      
      target_compile_definitions(timezone PRIVATE HAS_REMOTE_API=0)

      if (WIN32 )
         if(TOBASA_BUILD_IN_MEMORY_TZDB)
            target_compile_definitions(timezone PRIVATE USE_IN_MEMORY_TZDB=1)
         endif()
      else() 
         if(TOBASA_BUILD_IN_MEMORY_TZDB)
            target_compile_definitions(timezone PRIVATE USE_IN_MEMORY_TZDB=1)
         else()
            target_compile_definitions(timezone PRIVATE USE_OS_TZDB=1)
         endif()
      endif()

      if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
         target_compile_options(timezone PRIVATE -fpic -c -Wall)
      endif()

      if(CMAKE_CXX20_COMPILE_FEATURES)
         message(STATUS "tobasa use cxx_std_20")
         target_compile_features(timezone PUBLIC cxx_std_20)
      else()
         message(STATUS "tobasa use cxx_std_17")
         target_compile_features(timezone PUBLIC cxx_std_17)
      endif()      

   endif()
else()
   message(STATUS "tobasa Using std::date date library")
endif()


# -------------------------------------------------------
# The library
# -------------------------------------------------------
add_library(tobasa)
include(${PROJECT_SOURCE_DIR}/cmake/sources.cmake)

if(MSVC)
   set_property(TARGET ${PROJECT_NAME} PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()


target_sources(tobasa
   PRIVATE ${MY_SOURCES_LIST}
   PRIVATE ${MY_HEADERS_LIST})


target_include_directories(tobasa
   PUBLIC ${TOBASA_EXTERNAL_DEPS_DIR}/nlohmann/include
   PUBLIC ${TOBASA_EXTERNAL_DEPS_DIR}/utf8cpp/include
   PUBLIC ${PROJECT_SOURCE_DIR}/include )

   
if(NOT TOBASA_USE_STD_TEXT_FORMATTING)
   if(TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG)
      target_link_libraries(tobasa PUBLIC fmt::fmt )
   else()
      message(STATUS "tobasa Using bundled Formatting fmt library")
      target_link_libraries(tobasa PUBLIC fmt::fmt)
   endif()
endif()


if(NOT TOBASA_USE_STD_CALENDAR_TZONE)
   if(NOT TOBASA_USE_BUNDLED_TZ_LIB)
      target_link_libraries(tobasa PUBLIC date::date date::date-tz)
   else()
      target_include_directories(tobasa PUBLIC ${TOBASA_EXTERNAL_DEPS_DIR}/date/include )
      target_link_libraries(tobasa PUBLIC timezone )
      
      if(WIN32)
         if(TOBASA_BUILD_IN_MEMORY_TZDB)
            target_compile_definitions(tobasa PUBLIC TOBASA_DATE_USE_IN_MEMORY_TZDB)
         endif()
      else()
         if(TOBASA_BUILD_IN_MEMORY_TZDB)
            target_compile_definitions(tobasa PUBLIC TOBASA_DATE_USE_IN_MEMORY_TZDB)
         else()
            target_compile_definitions(tobasa PUBLIC TOBASA_DATE_USE_OS_TZDB)
         endif()
      endif()

   endif()
endif()


if(MSVC)
   target_compile_definitions(tobasa PUBLIC _CRT_SECURE_NO_WARNINGS _UNICODE NOMINMAX)
   target_compile_options(tobasa PRIVATE /bigobj /FS )
   target_link_libraries(tobasa PUBLIC shlwapi )
endif()


if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
   target_compile_options(tobasa PRIVATE -fpic -c -Wall)
endif()


if(CMAKE_CXX20_COMPILE_FEATURES)
   message(STATUS "tobasa use cxx_std_20")
   target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
else()
   message(STATUS "tobasa use cxx_std_17")
   target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
endif()


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