include(FetchContent)
include(CTest)
include(GoogleTest)

if(NOT TARGET tobasa_test_support)
   add_subdirectory(${CMAKE_SOURCE_DIR}/src/test_support
      ${CMAKE_BINARY_DIR}/src/test_support EXCLUDE_FROM_ALL)
endif()

set(TOBASA_GOOGLETEST_VERSION "v1.14.0" CACHE STRING "GoogleTest version used by all Tobasa modules")

if(NOT TARGET GTest::gtest_main)
   FetchContent_Declare(
      googletest
      URL "https://github.com/google/googletest/archive/refs/tags/${TOBASA_GOOGLETEST_VERSION}.tar.gz"
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
   )

   set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
   set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
   set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

   FetchContent_MakeAvailable(googletest)
endif()

function(tobasa_add_google_test target_name)
   set(options)
   set(oneValueArgs)
   set(multiValueArgs SOURCES)
   cmake_parse_arguments(TOBASA_GT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

   if(NOT TOBASA_GT_SOURCES)
      message(FATAL_ERROR "tobasa_add_google_test requires SOURCES argument")
   endif()

   add_executable(${target_name} ${TOBASA_GT_SOURCES})
   target_link_libraries(${target_name} PRIVATE GTest::gtest_main)

   set_target_properties(${target_name} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
      ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
   )

   gtest_discover_tests(${target_name})
endfunction()