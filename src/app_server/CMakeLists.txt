cmake_minimum_required(VERSION 3.12-3.18)
cmake_policy(SET CMP0091 NEW)

# The project
project(webservice LANGUAGES CXX)


option(TOBASA_FIND_DEPS "Use `find_package()` for including Tobasa dependencies." OFF)
option(TOBASA_BUILD_IN_MEMORY_RESOURCES "Whether to use compiled resources" OFF)


if(TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG)
   if(TOBASA_BUILD_TESTS_WSCHAT_USE_PROBUF OR TOBASA_BUILD_CHAT_MODULE)
      find_package(protobuf REQUIRED)
   endif()
else()
   if(TOBASA_BUILD_TESTS_WSCHAT_USE_PROBUF OR TOBASA_BUILD_CHAT_MODULE)
      set(PROTOBUF_INCLUDE_DIR ${TOBASA_PROTOBUF_DIR}/include)
      message(STATUS "webservice use protocol buffer include dir: ${PROTOBUF_INCLUDE_DIR}")
   endif()
endif()

if(TOBASA_BUILD_IN_MEMORY_RESOURCES)
   message(STATUS "tobasa Using in-memory resources")
endif()

if(NOT TOBASA_USE_STD_CALENDAR_TZONE AND TOBASA_BUILD_IN_MEMORY_TZDB)
   message(STATUS "tobasa Using in-memory timezone data")
endif()

# The executable
project(webservice)
add_executable(${PROJECT_NAME})
include(${PROJECT_SOURCE_DIR}/cmake/sources.cmake)

# Target Sources
# -------------------------------------------------------
target_sources( ${PROJECT_NAME}
   PRIVATE ${APP_SOURCES_LIST}
   PRIVATE ${APP_HEADERS_LIST}
   PRIVATE ${APP_HEADERS_RESOURCES_LIST}
   PRIVATE ${PROJECT_SOURCE_DIR}/src/build_info.cpp
   PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/resources/config_resources.cpp
   PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/resources/tls_asset_resources.cpp
   PRIVATE $<$<CXX_COMPILER_ID:MSVC>:${PROJECT_SOURCE_DIR}/src/tobasa_app.rc> )

if(TOBASA_BUILD_IN_MEMORY_RESOURCES)
   target_sources( ${PROJECT_NAME}
      PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/resources/template_resources.cpp
      PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/resources/wwwroot_resources.cpp )
endif()

if(TOBASA_BUILD_LIS_ENGINE)
   target_sources( ${PROJECT_NAME} 
      PRIVATE ${APP_LIS_SOURCES_LIST}
      PRIVATE ${APP_LIS_HEADERS_LIST} )
   
   if(TOBASA_BUILD_IN_MEMORY_RESOURCES)
      target_sources( ${PROJECT_NAME}
         PRIVATE ${APP_LIS_RESOURCES_HEADERS_LIST}
         PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/resources/template_lis_resources.cpp)
   endif()

endif()

if(TOBASA_BUILD_TESTS)
   target_sources( ${PROJECT_NAME} 
      PRIVATE ${APP_TEST_HEADERS_LIST}
      PRIVATE ${APP_TEST_SOURCES_LIST} )
   
   if (TOBASA_BUILD_TESTS_WSCHAT_USE_PROBUF)
      target_sources( ${PROJECT_NAME}
         PRIVATE ${APP_TEST_WSCHAT_PROBUF_HEADERS_LIST}
         PRIVATE ${APP_TEST_WSCHAT_PROBUF_SOURCES_LIST} )
   endif()
endif()

# Target link libraries, compile definitions, include dirs
# -------------------------------------------------------
target_link_libraries( ${PROJECT_NAME} PRIVATE tobasa tobasasql tobasahttp tobasaweb)

if (TOBASA_BUILD_LIS_ENGINE)   
   target_link_libraries( ${PROJECT_NAME} PRIVATE tobasalis )
   target_compile_definitions( ${PROJECT_NAME} PRIVATE TOBASA_USE_LIS_ENGINE)
endif()

target_include_directories( ${PROJECT_NAME}
   PRIVATE ${PROJECT_SOURCE_DIR}/src
   PRIVATE ${PROJECT_SOURCE_DIR}/src/antri
   PRIVATE ${PROJECT_SOURCE_DIR}/src/core
   PRIVATE ${PROJECT_SOURCE_DIR}/src/resources 
)

if(TOBASA_BUILD_TESTS)
   target_include_directories( ${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src/test )
   target_include_directories( ${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src/test_ws )
   target_compile_definitions( ${PROJECT_NAME} PRIVATE TOBASA_USE_TESTS_MODULE)
endif()

if(TOBASA_BUILD_TESTS_WSCHAT_USE_PROBUF OR TOBASA_BUILD_CHAT_MODULE)
   if(TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG)
      target_link_libraries(${PROJECT_NAME} PRIVATE protobuf::libprotobuf)
   else()
      if (MSVC)
         target_link_libraries( ${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:${TOBASA_PROTOBUF_DIR}/debug/lib/libprotobufd.lib>$<$<CONFIG:Release>:${TOBASA_PROTOBUF_DIR}/lib/libprotobuf.lib>  )
      else()
         target_link_libraries( ${PROJECT_NAME} PRIVATE ${TOBASA_PROTOBUF_DIR}/lib/libprotobuf.a )
      endif()
      
      target_include_directories( ${PROJECT_NAME} PRIVATE ${PROTOBUF_INCLUDE_DIR} )
   endif()

   if(TOBASA_BUILD_TESTS_WSCHAT_USE_PROBUF)
      target_compile_definitions( ${PROJECT_NAME} PRIVATE TOBASA_TEST_WSCHAT_USE_PROBUF )
   endif()

endif()

if(TOBASA_BUILD_IN_MEMORY_RESOURCES)
   target_compile_definitions( ${PROJECT_NAME} PRIVATE TOBASA_BUILD_IN_MEMORY_RESOURCES )
endif()


# -------------------------------------------------------
if(MSVC)
   set_property(TARGET ${PROJECT_NAME} PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

   target_link_libraries( ${PROJECT_NAME}
      PRIVATE ws2_32
      PRIVATE mswsock )

   target_compile_options(${PROJECT_NAME} PRIVATE /bigobj /FS)

   target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:/NODEFAULTLIB:LIBCMTD>)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
   target_compile_options(${PROJECT_NAME} PRIVATE -Wno-narrowing )
endif()

if(CMAKE_CXX20_COMPILE_FEATURES)
   message(STATUS "webservice use cxx_std_20")
   target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
else()
   message(STATUS "webservice use cxx_std_17")
   target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
endif()

# IDE Support
# -------------------------------------------------------
# Show filtered headers in IDE
source_group(
   TREE ${PROJECT_SOURCE_DIR}/src
   PREFIX "Header Files"
   FILES ${APP_HEADERS_LIST}
         ${APP_HEADERS_RESOURCES_LIST}
         ${APP_LIS_HEADERS_LIST} 
         ${APP_LIS_RESOURCES_HEADERS_LIST}
         ${APP_TEST_HEADERS_LIST}
         ${APP_TEST_WS_CHAT_PROTOBUF_HEADERS_LIST} )

# Show filtered sources in IDE
source_group(
   TREE ${PROJECT_SOURCE_DIR}/src
   PREFIX "Source Files"
   FILES ${APP_SOURCES_LIST}
         ${APP_LIS_SOURCES_LIST}
         ${APP_TEST_SOURCES_LIST}
         ${APP_TEST_WS_CHAT_PROTOBUF_SOURCES_LIST} )


# Pre Build and Post Build commands
# -------------------------------------------------------
# Generate and minify CSS - TODO: if this command ran, next commands will not get executed
#add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
#   COMMAND sass --style compressed --no-error-css ${PROJECT_SOURCE_DIR}/scss/styles.scss:${PROJECT_SOURCE_DIR}/wwwroot/css/styles.min.css )

# Application resources
# -------------------------------------------------------

# Copy appsettings.json and appsettings_header_rules.json to binary dir
if(EXISTS "${PROJECT_SOURCE_DIR}/configuration/appsettings.json" AND EXISTS "${PROJECT_SOURCE_DIR}/configuration/appsettings_header_rules.json")
   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/configuration"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
         ${PROJECT_SOURCE_DIR}/configuration/appsettings.json $<TARGET_FILE_DIR:${PROJECT_NAME}>/configuration/
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
         ${PROJECT_SOURCE_DIR}/configuration/appsettings_header_rules.json $<TARGET_FILE_DIR:${PROJECT_NAME}>/configuration/)   
endif()

# Copy web root (static resource) content to binary dir
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${PROJECT_SOURCE_DIR}/wwwroot/ $<TARGET_FILE_DIR:${PROJECT_NAME}>/wwwroot/)


# Copy template views content to binary dir
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${PROJECT_SOURCE_DIR}/views/ $<TARGET_FILE_DIR:${PROJECT_NAME}>/views/)


# Copy TLS certificates 
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_directory
   ${PROJECT_SOURCE_DIR}/tls_asset/ $<TARGET_FILE_DIR:${PROJECT_NAME}>/tls_asset/)


# Copy web data content to binary dir
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${PROJECT_SOURCE_DIR}/appdata/ $<TARGET_FILE_DIR:${PROJECT_NAME}>/appdata/)


# LIS Engine resources
# -------------------------------------------------------
if(TOBASA_BUILD_LIS_ENGINE)
   # Copy appsettings_lis.json to binary dir
   if(EXISTS "${PROJECT_SOURCE_DIR}/configuration/appsettings_lis.json")
      add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy
            ${PROJECT_SOURCE_DIR}/configuration/appsettings_lis.json $<TARGET_FILE_DIR:${PROJECT_NAME}>/configuration/)
   endif()

   # Copy template views content to binary dir
   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
         ${PROJECT_SOURCE_DIR}/views_lis/lis/ $<TARGET_FILE_DIR:${PROJECT_NAME}>/views/lis/)
endif()


# Copy libpq required shared libraries, only when linking dynamically with libpq from PostgreSql binary dist
# If we link with statically compiled libpq, we don't need these dlls
# -------------------------------------------------------
if(TOBASA_SQL_USE_PGSQL AND MSVC AND NOT TOBASA_BUILD_WITH_VCPKG)
   # For Pgsql 11.7 x64
   if( ${TOBASA_LIBPQ_DIR} STREQUAL "D:/programs/PostgreSQL/11.7_x64" )
      message(STATUS "webservice use libpq from: ${TOBASA_LIBPQ_DIR} - ${PROJECT_NAME}")
      set(LIBPQ_BIN_DLLS libcrypto-1_1-x64.dll libiconv-2.dll libintl-9.dll libpq.dll libssl-1_1-x64.dll libwinpthread-1.dll)

      foreach(DLL IN LISTS LIBPQ_BIN_DLLS)
         add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${TOBASA_LIBPQ_DIR}/bin/${DLL} $<TARGET_FILE_DIR:${PROJECT_NAME}>/)
      endforeach()
   endif()

   # For Pgsql 10.19 x32
   if( ${TOBASA_LIBPQ_DIR} STREQUAL "D:/programs/PostgreSQL/10.19_x86" )
      message(STATUS "webservice use libpq from: ${TOBASA_LIBPQ_DIR} - ${PROJECT_NAME}")
      set(LIBPQ_BIN_DLLS libpq.dll libintl-8.dll libiconv-2.dll libcrypto-1_1.dll libssl-1_1.dll)

      foreach(DLL IN LISTS LIBPQ_BIN_DLLS)
         add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${TOBASA_LIBPQ_DIR}/bin/${DLL} $<TARGET_FILE_DIR:${PROJECT_NAME}>/)
      endforeach()
   endif()
endif()


# Copy libmariadb.dll
# -------------------------------------------------------
if(TOBASA_SQL_USE_MYSQL AND TOBASA_SQL_USE_MARIADB_DLL)
   if(MSVC AND NOT TOBASA_BUILD_WITH_VCPKG)
      add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy
         ${TOBASA_FRAMEWORK_BINARY_DIR}/bin/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>/libmariadb.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>/)
   endif()
endif()


# Copy Timezone database
# -------------------------------------------------------
if(NOT TOBASA_USE_STD_CALENDAR_TZONE)
   if(NOT TOBASA_BUILD_IN_MEMORY_TZDB)
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

   # Remove unnecessary files/folders
   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E rm -rf ${TOBASA_PACKAGE_OUTPUT_DIR}/${PROJECT_NAME}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>/resources
      COMMAND ${CMAKE_COMMAND} -E rm -rf ${TOBASA_PACKAGE_OUTPUT_DIR}/${PROJECT_NAME}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>/CMakeFiles
      COMMAND ${CMAKE_COMMAND} -E rm -f ${TOBASA_PACKAGE_OUTPUT_DIR}/${PROJECT_NAME}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>/cmake_install.cmake
      COMMAND ${CMAKE_COMMAND} -E rm -f ${TOBASA_PACKAGE_OUTPUT_DIR}/${PROJECT_NAME}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>/Makefile )
endif()