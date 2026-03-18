cmake_minimum_required(VERSION 3.12-3.18)
cmake_policy(SET CMP0091 NEW)
include(CMakeDependentOption)

# The project
project(tobasasql VERSION 1.0 LANGUAGES CXX)

if(NOT DEFINED TOBASA_EXTERNAL_DEPS_DIR)
   set(TOBASA_EXTERNAL_DEPS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ext")
endif()

option(TOBASA_FIND_DEPS    "Use `find_package()` for including Tobasa SQL dependencies." OFF)

option(TOBASA_SQL_USE_PGSQL    "Enable Tobasa PostgreSQL driver" OFF)
option(TOBASA_SQL_USE_ADODB    "Enable Tobasa ADO driver"        OFF) # Only with MSVC compiler
option(TOBASA_SQL_USE_ODBC     "Enable Tobasa ODBC driver"       OFF)
option(TOBASA_SQL_USE_SQLITE   "Enable Tobasa SQLite driver"     OFF)
option(TOBASA_SQL_USE_MYSQL    "Enable TobasaMariaDB driver"     OFF)

CMAKE_DEPENDENT_OPTION(TOBASA_SQL_USE_MARIADB_DLL "Use libmariadb shared (.dll) library" ON "TOBASA_SQL_USE_MYSQL" OFF)

# -------------------------------------------------------
# SQLite3 library
# -------------------------------------------------------
if(NOT TOBASA_SQL_USE_SQLITE)
   set(TOBASA_SQL_USE_SQLITE ON CACHE BOOL "Enable Tobasa SQLite driver" FORCE)
endif()
if(TOBASA_SQL_USE_SQLITE)
   if(TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG)
      set(TOBASA_SQL_USE_SQLITE3_MC OFF)
   else()
      set(TOBASA_SQL_USE_SQLITE3_MC ON)
   endif()

   if(TOBASA_SQL_USE_SQLITE3_MC)
      # SQLite3 Multiple Chipers sources included
      set(TOBASA_SQLITE3_LIB "sqlite3_mc")
      add_subdirectory(${TOBASA_EXTERNAL_DEPS_DIR}/sqlite3_mc ${CMAKE_SOURCE_DIR}/build_ext/sqlite3_mc EXCLUDE_FROM_ALL)
   else()
      # SQLite3 library via find_package
      find_package(unofficial-sqlite3 CONFIG REQUIRED)
      set(TOBASA_SQLITE3_LIB "unofficial::sqlite3::sqlite3")
   endif()
   message(STATUS "tobasasql sqlite3 lib: ${TOBASA_SQLITE3_LIB}")
endif()


# -------------------------------------------------------
# PostgreSQL library
# -------------------------------------------------------
if(TOBASA_SQL_USE_PGSQL)
   if(TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG)
      set(TOBASA_SQL_USE_PGSQL_CUSTOM OFF)
   else()
      if(MSVC)
         set(TOBASA_SQL_USE_PGSQL_CUSTOM ON)
      else()
         set(TOBASA_SQL_USE_PGSQL_CUSTOM OFF)
      endif()
   endif()

   if(TOBASA_SQL_USE_PGSQL_CUSTOM AND MSVC)
      if(NOT DEFINED TOBASA_LIBPQ_DIR)
         set(TOBASA_LIBPQ_DIR ${TOBASA_LIBPQ_DIR_DEF} )
      endif()

      if(NOT DEFINED TOBASA_OPENSSL_DIR)
         set(TOBASA_OPENSSL_DIR ${TOBASA_OPENSSL_DIR_DEF} )
      endif()
      if(DEFINED TOBASA_OPENSSL_DIR)
         # use openssl custom folder
         set(ENV{OPENSSL_ROOT_DIR} ${TOBASA_OPENSSL_DIR} )
      endif()

      if(${TOBASA_LIBPQ_DIR} STREQUAL "D:/programs/PostgreSQL/11.7_x64"  OR ${TOBASA_LIBPQ_DIR} STREQUAL "D:/programs/PostgreSQL/10.19_x86")
         # Link dynamically with libpq
         set(TOBASA_PGSQL_LIBRARY ${TOBASA_LIBPQ_DIR}/lib/libpq.lib) 
      else()
         # Link with statically built libpq
         set(TOBASA_PGSQL_LIBRARY wldap32.lib 
            optimized ${TOBASA_LIBPQ_DIR}/lib/libpq.lib debug ${TOBASA_LIBPQ_DIR}/debug/lib/libpq.lib 
            optimized ${TOBASA_LIBPQ_DIR}/lib/libpgport.lib debug ${TOBASA_LIBPQ_DIR}/debug/lib/libpgport.lib
            optimized ${TOBASA_LIBPQ_DIR}/lib/libpgcommon.lib debug ${TOBASA_LIBPQ_DIR}/debug/lib/libpgcommon.lib
            optimized ${TOBASA_OPENSSL_DIR}/lib/libssl.lib debug ${TOBASA_OPENSSL_DIR}/debug/lib/libssl.lib
            optimized ${TOBASA_OPENSSL_DIR}/lib/libcrypto.lib debug ${TOBASA_OPENSSL_DIR}/debug/lib/libcrypto.lib
         )
      endif()
      
      set(TOBASA_PGSQL_INCLUDE_DIR ${TOBASA_LIBPQ_DIR}/include)

   else()
      find_package(PostgreSQL REQUIRED)
      set(TOBASA_PGSQL_INCLUDE_DIR ${PostgreSQL_INCLUDE_DIRS})
      set(TOBASA_PGSQL_LIBRARY     PostgreSQL::PostgreSQL )
   endif()

   message(STATUS "tobasasql PGSQL include dir : ${TOBASA_PGSQL_INCLUDE_DIR}")
   message(STATUS "tobasasql PGSQL library dir : ${PostgreSQL_LIBRARY_DIRS}")
endif()


# -------------------------------------------------------
# ODBC library
# -------------------------------------------------------
if(TOBASA_SQL_USE_ODBC)
   find_package(ODBC REQUIRED)
   if(ODBC_FOUND)
      message(STATUS "tobasasql ODBC include dir : ${ODBC_INCLUDE_DIRS}")
      message(STATUS "tobasasql ODBC libraries   : ${ODBC_LIBRARIES}")
   else()
      message(STATUS "tobasasql ODBC library     : Not found")
   endif()
endif()


# -------------------------------------------------------
# MARIADB C CONNECTOR LIBRARY
# -------------------------------------------------------
if(TOBASA_SQL_USE_MYSQL)
   if(TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG)
      set(TOBASA_SQL_USE_BUNDLED_LIBMARIADB OFF)
   else()
      set(TOBASA_SQL_USE_BUNDLED_LIBMARIADB ON)
      if(TOBASA_USE_PREBUILT_ZLIB)
         set(ZLIB_LIBS_MARIADB optimized ${TOBASA_ZLIB_DIR}/lib/zlib_o.lib debug ${TOBASA_ZLIB_DIR}/lib/zlib_d.lib )
         set(ZLIB_INC_DIR_MARIADB ${TOBASA_ZLIB_DIR}/include)
      elseif(TOBASA_USE_BUNDLED_ZLIB)
         set(ZLIB_LIBS_MARIADB ZLIB::ZLIBSTATIC )
         set(ZLIB_INC_DIR_MARIADB ${CMAKE_BINARY_DIR}/include/zlib )
      else()
         if(NOT ZLIB_FOUND)
            find_package(ZLIB REQUIRED)
         endif()
         set(ZLIB_LIBS_MARIADB ${ZLIB_LIBRARIES})
         set(ZLIB_INC_DIR_MARIADB ${ZLIB_INCLUDE_DIR})
      endif()
   endif()


   if(TOBASA_SQL_USE_BUNDLED_LIBMARIADB)
      # MariaDB MariaDB Connector C sources included
      set(TOBASA_MARIADB_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include)

      set(TOBASA_MARIADB_LIB mariadbclient ${ZLIB_LIBS_MARIADB}) # static library
      if(TOBASA_SQL_USE_MARIADB_DLL)
         set(TOBASA_MARIADB_LIB libmariadb ${ZLIB_LIBS_MARIADB}) # dynamic lib
      endif()

      # set mariadb-connector cmake options
      set(INSTALL_INCLUDEDIR include/mysql)
      set(INSTALL_PLUGINDIR plugins/libmariadb) # TO avoid error at src/ext/mariadb-connector-c/cmake/install_plugins.cmake:11
      set(WITH_CURL OFF)
      set(WITH_UNIT_TESTS OFF)
      set(WITH_EXTERNAL_ZLIB ON)
      set(ZLIB_LIBRARIES ${ZLIB_LIBS_MARIADB})
      set(ZLIB_INCLUDE_DIR ${ZLIB_INC_DIR_MARIADB})

      message(STATUS "tobasasql use internal bundled MariaDB Connector C ")
      add_subdirectory(${TOBASA_EXTERNAL_DEPS_DIR}/mariadb-connector-c ${CMAKE_SOURCE_DIR}/build_ext/mariadb-connector-c EXCLUDE_FROM_ALL)
   else()
      find_package(unofficial-libmariadb CONFIG REQUIRED)
      set(TOBASA_MARIADB_LIB unofficial::mariadbclient) # static library
      if(TOBASA_SQL_USE_MARIADB_DLL)
         set(TOBASA_MARIADB_LIB unofficial::libmariadb) # dynamic lib
      endif()
   endif()
   message(STATUS "tobasasql MariaDB libraries    : ${TOBASA_MARIADB_LIB}")
   message(STATUS "tobasasql ZLIB_LIBS_MARIADB    : ${ZLIB_LIBRARIES}")
   message(STATUS "tobasasql ZLIB_INC_DIR_MARIADB : ${ZLIB_INCLUDE_DIR}")
endif()


# -------------------------------------------------------
# The library
# -------------------------------------------------------
add_library(${PROJECT_NAME})
include(${PROJECT_SOURCE_DIR}/cmake/sources.cmake)

if(MSVC)
   set_property(TARGET ${PROJECT_NAME} PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

target_sources(${PROJECT_NAME}
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

target_link_libraries(${PROJECT_NAME} PRIVATE tobasa)

if(TOBASA_SQL_USE_PGSQL)
   if(TOBASA_SQL_USE_PGSQL_CUSTOM AND MSVC)
      target_include_directories(${PROJECT_NAME} PUBLIC ${TOBASA_PGSQL_INCLUDE_DIR})
      #target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:/NODEFAULTLIB:LIBCMTD>)
   endif()

   target_link_libraries(${PROJECT_NAME} PUBLIC ${TOBASA_PGSQL_LIBRARY})
   target_compile_definitions(${PROJECT_NAME} PUBLIC TOBASA_SQL_USE_PGSQL)
endif()

if(TOBASA_SQL_USE_ADODB AND MSVC)
   target_compile_definitions(${PROJECT_NAME} PUBLIC TOBASA_SQL_USE_ADODB)
endif()

if(TOBASA_SQL_USE_SQLITE)
   target_link_libraries(${PROJECT_NAME} PUBLIC ${TOBASA_SQLITE3_LIB})
   target_compile_definitions(${PROJECT_NAME} PUBLIC TOBASA_SQL_USE_SQLITE)
endif()

if(TOBASA_SQL_USE_ODBC)
   target_link_libraries(${PROJECT_NAME} PRIVATE ${ODBC_LIBRARIES})
   target_compile_definitions(${PROJECT_NAME} PUBLIC TOBASA_SQL_USE_ODBC)
   if(MSVC)
      target_compile_definitions(${PROJECT_NAME} PUBLIC WIN32_LEAN_AND_MEAN )
   endif()
endif()

if(TOBASA_SQL_USE_MYSQL)
   target_link_libraries(${PROJECT_NAME} PUBLIC ${TOBASA_MARIADB_LIB})   
   
   if(TOBASA_SQL_USE_BUNDLED_LIBMARIADB)
      if (MSVC)
         target_include_directories(${PROJECT_NAME} PUBLIC ${TOBASA_MARIADB_INCLUDE_DIR})
      else()
         target_include_directories(${PROJECT_NAME} PUBLIC ${TOBASA_MARIADB_INCLUDE_DIR})
      endif()
   endif()
   
   target_compile_definitions(${PROJECT_NAME} PUBLIC TOBASA_SQL_USE_MYSQL)
   if(TOBASA_SQL_USE_MARIADB_DLL)
      target_compile_definitions(${PROJECT_NAME} PUBLIC TOBASA_SQL_USE_MYSQL TOBASA_SQL_USE_MARIADB_DLL)
   endif()
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include )


if(MSVC)
   # Add system libraries
   if(NOT (TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG))
      target_link_libraries(${PROJECT_NAME} PUBLIC ws2_32 mswsock secur32 crypt32 shlwapi advapi32 kernel32)
   endif()
   
   target_compile_options(${PROJECT_NAME} PRIVATE /permissive /bigobj /FS )
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
   target_compile_options(${PROJECT_NAME} PRIVATE -fpic -c -Wall )
endif()

if(CMAKE_CXX20_COMPILE_FEATURES)
   message(STATUS "${PROJECT_NAME} use cxx_std_20")
   target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
else()
   message(STATUS "${PROJECT_NAME} use cxx_std_17")
   target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
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