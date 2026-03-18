cmake_minimum_required(VERSION 3.12-3.18)
cmake_policy(SET CMP0091 NEW)

project(http_server LANGUAGES CXX)
add_executable(${PROJECT_NAME})
include(${PROJECT_SOURCE_DIR}/cmake/sources.cmake)

target_sources( ${PROJECT_NAME}
   PRIVATE ${MY_SOURCES_LIST}
   PRIVATE ${MY_HEADERS_LIST}
   PRIVATE $<$<CXX_COMPILER_ID:MSVC>:${PROJECT_SOURCE_DIR}/src/tobasa_app.rc> )

target_link_libraries( ${PROJECT_NAME}
   PRIVATE tobasa tobasahttp)

target_include_directories( ${PROJECT_NAME}
   PRIVATE ${PROJECT_SOURCE_DIR}/src )

if (MSVC)
   set_property(TARGET ${PROJECT_NAME} PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

   target_compile_definitions( ${PROJECT_NAME}
      PRIVATE _CRT_SECURE_NO_WARNINGS
      PRIVATE WIN32_LEAN_AND_MEAN )
else()
   if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
      target_compile_options(${PROJECT_NAME} PRIVATE -pthread )
      target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
   endif()
endif()

if(CMAKE_CXX20_COMPILE_FEATURES)
   message(STATUS "${PROJECT_NAME} use cxx_std_20")
   target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
else()
   message(STATUS "${PROJECT_NAME} use cxx_std_17")
   target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
endif()

if(MSVC)
   target_compile_options(${PROJECT_NAME} PRIVATE /bigobj /FS )
endif()

# IDE Support
# ------------------------------------------------------
# Show filtered headers in IDE
source_group(
   TREE ${PROJECT_SOURCE_DIR}/src
   PREFIX "Header Files"
   FILES ${MY_HEADERS_LIST} )

# Show filtered sources in IDE
source_group(
   TREE ${PROJECT_SOURCE_DIR}/src
   PREFIX "Source Files"
   FILES ${MY_SOURCES_LIST} )

# Pre Build and Post Build commands
# ------------------------------------------------------
# Copy web root content to binary dir
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${PROJECT_SOURCE_DIR}/wwwroot/ $<TARGET_FILE_DIR:${PROJECT_NAME}>/wwwroot/)

# Copy TLS certificates
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${PROJECT_SOURCE_DIR}/resources/ $<TARGET_FILE_DIR:${PROJECT_NAME}>/)


# Copy Timezone database
# -------------------------------------------------------
if(NOT TOBASA_USE_STD_CALENDAR_TZONE)
   if (NOT TOBASA_BUILD_IN_MEMORY_TZDB)
      add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy_directory
         ${TOBASA_FRAMEWORK_ROOTDIR}/res/tzdata/ $<TARGET_FILE_DIR:${PROJECT_NAME}>/tzdata/)
   endif()
endif()

# Copy binary dir to _output dir
# -------------------------------------------------------
if(TOBASA_CREATE_PACKAGE_OUTPUT_DIR)
   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
         $<TARGET_FILE_DIR:${PROJECT_NAME}>/ ${TOBASA_PACKAGE_OUTPUT_DIR}/${PROJECT_NAME}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>/ )

   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E rm -rf ${TOBASA_PACKAGE_OUTPUT_DIR}/${PROJECT_NAME}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>/CMakeFiles )

   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E rm -f ${TOBASA_PACKAGE_OUTPUT_DIR}/${PROJECT_NAME}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>/cmake_install.cmake )
      
   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E rm -f ${TOBASA_PACKAGE_OUTPUT_DIR}/${PROJECT_NAME}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>/Makefile )
endif()