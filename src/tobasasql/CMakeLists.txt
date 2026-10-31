cmake_minimum_required(VERSION 3.14-3.18)
cmake_policy(SET CMP0091 NEW)
if(TOBASA_BUILD_MONOLITHIC_SHARED_LIB)
   cmake_policy(SET CMP0079 NEW)
endif()
include(CMakeDependentOption)

# The project
project(tobasasql VERSION 1.0 LANGUAGES CXX)

if(NOT DEFINED TOBASA_EXTERNAL_DEPS_DIR)
   set(TOBASA_EXTERNAL_DEPS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ext")
endif()

option(TOBASA_FIND_DEPS    "Use `find_package()` for including Tobasa SQL dependencies." OFF)

option(TOBASA_SQL_USE_PGSQL      "Enable PostgreSQL driver"           ON )
option(TOBASA_SQL_USE_ADODB      "Enable ADO driver"                  ON )
option(TOBASA_SQL_USE_ODBC       "Enable ODBC driver"                 ON )
option(TOBASA_SQL_USE_SQLITE     "Enable SQLite driver"               ON )
option(TOBASA_SQL_USE_MYSQL      "Enable MariaDB driver"              ON )

option(TOBASA_SQL_USE_PREBUILT_MARIADB "Use prebuilt MariaDB Connector/C library"  OFF)
option(TOBASA_SQL_USE_PREBUILT_LIBPQ   "Use prebuilt Libpq"  OFF)

# -------------------------------------------------------
# SQLite3 library
# -------------------------------------------------------
if(NOT TOBASA_SQL_USE_SQLITE)
   set(TOBASA_SQL_USE_SQLITE ON CACHE BOOL "Enable Tobasa SQLite driver" FORCE)
endif()
if(TOBASA_SQL_USE_SQLITE)
   if(TOBASA_SQLITE3_FOUND)
      message(STATUS "tobasasql SQLite3 lib           : ${TOBASA_SQLITE3_LIBRARIES}")
   else()
      include(cmake/tobasa_sqlite3.cmake)
      tobasa_configure_sqlite3()
   endif()
endif()


# -------------------------------------------------------
# PostgreSQL library
# -------------------------------------------------------
if(TOBASA_SQL_USE_PGSQL)
   include(cmake/tobasa_libpq.cmake)
   tobasa_configure_libpq()
endif()


# -------------------------------------------------------
# ODBC library
# -------------------------------------------------------
if(TOBASA_SQL_USE_ODBC)
   find_package(ODBC REQUIRED)
   if(ODBC_FOUND)
      message(STATUS "tobasasql ODBC include dir      : ${ODBC_INCLUDE_DIRS}")
      message(STATUS "tobasasql ODBC libraries        : ${ODBC_LIBRARIES}")
   else()
      message(STATUS "tobasasql ODBC library          : Not found")
   endif()
endif()


# -------------------------------------------------------
# MARIADB C CONNECTOR LIBRARY
# -------------------------------------------------------
if(TOBASA_SQL_USE_MYSQL)
   include(cmake/tobasa_mariadb.cmake)
   tobasa_configure_mariadb_client()
endif()


# -------------------------------------------------------
# The library
# -------------------------------------------------------
if(TOBASA_BUILD_MONOLITHIC_SHARED_LIB)
   set(TOBASA_MODULE_TARGET tobasa_monolithic)
   target_compile_definitions(${TOBASA_MODULE_TARGET} PRIVATE TBS_SQL_SHARED)
elseif(TOBASA_BUILD_SHARED_LIB)
   set(TOBASA_MODULE_TARGET ${PROJECT_NAME})
   add_library(${PROJECT_NAME} SHARED)
   target_compile_definitions(${TOBASA_MODULE_TARGET} PRIVATE TBS_SQL_SHARED)
else()
   set(TOBASA_MODULE_TARGET ${PROJECT_NAME})
   add_library(${PROJECT_NAME} STATIC)
   target_compile_definitions(${TOBASA_MODULE_TARGET} PUBLIC TBS_SQL_STATIC)
endif()

include(${PROJECT_SOURCE_DIR}/cmake/sources.cmake)

target_sources(${TOBASA_MODULE_TARGET}
   PRIVATE ${TOBASASQL_CORE_SOURCES_LIST}
   PRIVATE ${TOBASASQL_CORE_HEADERS_LIST}
   PRIVATE ${TOBASASQL_PGSQL_SOURCES_LIST}
   PRIVATE ${TOBASASQL_PGSQL_HEADERS_LIST}
   PRIVATE ${TOBASASQL_ADODB_SOURCES_LIST}
   PRIVATE ${TOBASASQL_ADODB_HEADERS_LIST}
   PRIVATE ${TOBASASQL_ODBC_SOURCES_LIST}
   PRIVATE ${TOBASASQL_ODBC_HEADERS_LIST}
   PRIVATE ${TOBASASQL_SQLITE_SOURCES_LIST}
   PRIVATE ${TOBASASQL_SQLITE_HEADERS_LIST}
   PRIVATE ${TOBASASQL_MARIADB_SOURCES_LIST}
   PRIVATE ${TOBASASQL_MARIADB_HEADERS_LIST}
)

if(NOT TOBASA_BUILD_MONOLITHIC_SHARED_LIB)
   target_link_libraries(${TOBASA_MODULE_TARGET} PUBLIC tobasa)
endif()

if(TOBASA_SQL_USE_PGSQL)
   target_include_directories(${TOBASA_MODULE_TARGET} PUBLIC ${TOBASA_LIBPQ_INCLUDE_DIRS})
   target_link_libraries(${TOBASA_MODULE_TARGET} PUBLIC ${TOBASA_LIBPQ_LIBRARIES})
   target_compile_definitions(${TOBASA_MODULE_TARGET} PUBLIC TOBASA_SQL_USE_PGSQL)
endif()

if(TOBASA_SQL_USE_ADODB AND MSVC)
   target_compile_definitions(${TOBASA_MODULE_TARGET} PUBLIC TOBASA_SQL_USE_ADODB)
endif()

if(TOBASA_SQL_USE_SQLITE)
   target_link_libraries(${TOBASA_MODULE_TARGET} PUBLIC ${TOBASA_SQLITE3_LIBRARIES})
   target_include_directories(${TOBASA_MODULE_TARGET} PUBLIC ${TOBASA_SQLITE3_INCLUDE_DIRS} )
   target_compile_definitions(${TOBASA_MODULE_TARGET} PUBLIC TOBASA_SQL_USE_SQLITE)
   if(TOBASA_USE_SQLITE3_MC)
      target_compile_definitions(${TOBASA_MODULE_TARGET} PUBLIC TOBASA_SQL_USE_SQLITE3_MC)
   endif()
endif()

if(TOBASA_SQL_USE_ODBC)
   target_link_libraries(${TOBASA_MODULE_TARGET} PRIVATE ${ODBC_LIBRARIES})
   target_compile_definitions(${TOBASA_MODULE_TARGET} PUBLIC TOBASA_SQL_USE_ODBC)
   if(MSVC)
      target_compile_definitions(${TOBASA_MODULE_TARGET} PUBLIC WIN32_LEAN_AND_MEAN )
   endif()
endif()

if(TOBASA_SQL_USE_MYSQL)
   target_link_libraries(${TOBASA_MODULE_TARGET} PUBLIC ${TOBASA_MARIADB_LIBRARIES})
   target_include_directories(${TOBASA_MODULE_TARGET} PUBLIC ${TOBASA_MARIADB_INCLUDE_DIRS})
   target_compile_definitions(${TOBASA_MODULE_TARGET} PUBLIC TOBASA_SQL_USE_MYSQL)
   if(TOBASA_SQL_USE_MARIADB_DLL)
      target_compile_definitions(${TOBASA_MODULE_TARGET} PUBLIC TOBASA_SQL_USE_MYSQL TOBASA_SQL_USE_MARIADB_DLL)
   endif()
endif()

target_include_directories(${TOBASA_MODULE_TARGET} PUBLIC ${PROJECT_SOURCE_DIR}/include )

if(TOBASA_BUILD_TESTS)
   add_subdirectory(tests)
endif()


if(MSVC)
   # Add system libraries
   if(NOT (TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG))
      target_link_libraries(${TOBASA_MODULE_TARGET} PUBLIC ws2_32 mswsock secur32 crypt32 shlwapi advapi32 kernel32)
   endif()
   
   target_compile_options(${TOBASA_MODULE_TARGET} PRIVATE /permissive /bigobj /FS )
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
   target_compile_options(${TOBASA_MODULE_TARGET} PRIVATE -fpic -c -Wall )
endif()

if(CMAKE_CXX20_COMPILE_FEATURES)
   message(STATUS "${PROJECT_NAME} use cxx_std_20")
   target_compile_features(${TOBASA_MODULE_TARGET} PUBLIC cxx_std_20)
else()
   message(STATUS "${PROJECT_NAME} use cxx_std_17")
   target_compile_features(${TOBASA_MODULE_TARGET} PUBLIC cxx_std_17)
endif()

# Show filtered sources in IDE
source_group(
   TREE ${PROJECT_SOURCE_DIR}/src
   PREFIX "Source Files"
   FILES ${TOBASASQL_CORE_SOURCES_LIST}
         ${TOBASASQL_PGSQL_SOURCES_LIST}
         ${TOBASASQL_ADODB_SOURCES_LIST}
         ${TOBASASQL_ODBC_SOURCES_LIST}
         ${TOBASASQL_SQLITE_SOURCES_LIST}
         ${TOBASASQL_MARIADB_SOURCES_LIST}
   )

# Show filtered headers in IDE
source_group(
   TREE ${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}
   PREFIX "Header Files"
   FILES ${TOBASASQL_CORE_HEADERS_LIST}
         ${TOBASASQL_PGSQL_HEADERS_LIST}
         ${TOBASASQL_ADODB_HEADERS_LIST}
         ${TOBASASQL_ODBC_HEADERS_LIST}
         ${TOBASASQL_SQLITE_HEADERS_LIST}
         ${TOBASASQL_MARIADB_HEADERS_LIST}
   )