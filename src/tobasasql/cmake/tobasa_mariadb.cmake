macro(tobasa_configure_mariadb_client)

   set(MARIADB_VERSION_TAG "v3.4.11")

   check_cmake_project("${TOBASA_EXTERNAL_DEPS_DIR}/mariadb-connector-c" "${MARIADB_VERSION_TAG}" _has_bundled_mariadb)
   if(_has_bundled_mariadb)
      message(STATUS "tobasa Has bundled MariaDB Connector/C")
   endif()

   if(TOBASA_SQL_USE_PREBUILT_MARIADB)
      message(STATUS "tobasa Using prebuilt MariaDB Connector/C library")

      set(TOBASA_MARIADB_INCLUDE_DIRS ${TOBASA_MARIADB_DIR}/include)
      if(TOBASA_SQL_USE_MARIADB_DLL)
         set(TOBASA_MARIADB_LIBRARIES optimized ${TOBASA_MARIADB_DIR}/lib/libmariadb.lib debug ${TOBASA_MARIADB_DIR}/debug/lib/libmariadb.lib ${TOBASA_ZLIB_LIBRARIES})
      else()
         set(TOBASA_MARIADB_LIBRARIES optimized ${TOBASA_MARIADB_DIR}/lib/mariadbclient.lib debug ${TOBASA_MARIADB_DIR}/debug/lib/mariadbclient.lib ${TOBASA_ZLIB_LIBRARIES})
      endif()
      set(TOBASA_MARIADB_FOUND TRUE)

   elseif(TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG)
      if(TOBASA_BUILD_WITH_VCPKG)
         message(STATUS "tobasa Finding MariaDB Connector/C with find_package VCPKG")

         find_package(unofficial-libmariadb CONFIG REQUIRED)
         set(TOBASA_MARIADB_INCLUDE_DIRS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include")
         if(TOBASA_SQL_USE_MARIADB_DLL)
            set(TOBASA_MARIADB_LIBRARIES "unofficial::libmariadb")    # dynamic lib
         else()
            set(TOBASA_MARIADB_LIBRARIES "unofficial::mariadbclient") # static library
         endif()
         set(TOBASA_MARIADB_FOUND TRUE)

      else()
         message(STATUS "tobasa Finding MariaDB Connector/C with find_package")

         find_package(PkgConfig REQUIRED)
         pkg_check_modules(MARIADB REQUIRED IMPORTED_TARGET libmariadb)

         set(TOBASA_MARIADB_INCLUDE_DIRS "")
         set(TOBASA_MARIADB_LIBRARIES "PkgConfig::MARIADB")
         set(TOBASA_MARIADB_FOUND TRUE)
      endif()

   else()
      # Build from source
      set_and_save_cmake_pic_state()

      if(MSVC AND DEFINED TOBASA_OPENSSL_DIR)
         set(ENV{OPENSSL_ROOT_DIR} ${TOBASA_OPENSSL_DIR})
      endif()

      # Build options
      # -----------------------------------------------------------
      set(WITH_CURL           OFF CACHE BOOL "" FORCE)
      set(WITH_UNIT_TESTS     OFF CACHE BOOL "" FORCE)
      set(WITH_TOOLS          OFF CACHE BOOL "" FORCE)
      set(WITH_DOCS           OFF CACHE BOOL "" FORCE)
      set(WITH_EXTERNAL_ZLIB  ON  CACHE BOOL "" FORCE)
      set(ZLIB_LIBRARIES      "${TOBASA_ZLIB_LIBRARIES}")
      set(ZLIB_INCLUDE_DIR    "${TOBASA_ZLIB_INCLUDE_DIRS}")
      set(INSTALL_INCLUDEDIR  include/mysql)
      set(INSTALL_PLUGINDIR   plugins/libmariadb) # TO avoid error at src/ext/mariadb-connector-c/cmake/install_plugins.cmake:11
      #set(CLIENT_PLUGIN_mysql_clear_password DYNAMIC)

      find_package(OpenSSL REQUIRED)              # Use OpenSSL for TLS.
      set(WITH_SSL OPENSSL CACHE STRING "" FORCE)

      # -----------------------------------------------------------

      if(_has_bundled_mariadb)
         set(TOBASA_SQL_USE_BUNDLED_MARIADB TRUE)
         message(STATUS "tobasa Using bundled MariaDB Connector/C library")
         #set(mariadb-connector-c_SOURCE_DIR "${TOBASA_EXTERNAL_DEPS_DIR}/mariadb-connector-c")
         #set(mariadb-connector-c_BINARY_DIR "${CMAKE_BINARY_DIR}/_exts/mariadb-connector-c")
         add_subdirectory(
            "${TOBASA_EXTERNAL_DEPS_DIR}/mariadb-connector-c"
            "${CMAKE_BINARY_DIR}/_exts/mariadb-connector-c"
            EXCLUDE_FROM_ALL )
      else()
         message(STATUS "tobasa Fetching MariaDB Connector/C library")
         include(FetchContent)
         FetchContent_Declare(
            mariadb-connector-c
            GIT_REPOSITORY https://github.com/mariadb-corporation/mariadb-connector-c.git
            GIT_TAG        "${MARIADB_VERSION_TAG}"
            GIT_SHALLOW    TRUE
         )
         FetchContent_MakeAvailable(mariadb-connector-c)
      endif()

      if(WIN32)
         foreach(_target IN ITEMS mysql_native_password dialog client_ed25519 caching_sha2_password sha256_password parsec auth_gssapi_client mysql_old_password mysql_clear_password)
            if(TARGET ${_target})
               target_link_libraries(${_target} ws2_32 crypt32.lib)
            endif()
         endforeach()
      endif()

      set(MARIADB_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/include")

      # Prepare mariadb client header files for dependent projects and building examples
      # -----------------------------------------------------------
      set(_MARIADB_INCLUDE_DIR       "${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}")
      set(MARIADB_BINARY_INCLUDE_DIR "${mariadb-connector-c_BINARY_DIR}/include")
      set(MARIADB_SOURCE_INCLUDE_DIR "${mariadb-connector-c_SOURCE_DIR}/include")

      message(STATUS "tobasa MariaDB source include   : ${MARIADB_SOURCE_INCLUDE_DIR}")
      message(STATUS "tobasa MariaDB binary include   : ${MARIADB_BINARY_INCLUDE_DIR}")
      message(STATUS "tobasa MariaDB destination      : ${_MARIADB_INCLUDE_DIR}")

      file(MAKE_DIRECTORY "${_MARIADB_INCLUDE_DIR}/mysql" "${_MARIADB_INCLUDE_DIR}/mariadb")

      # Copy top-level public headers.
      file(GLOB MARIADB_PUBLIC_HEADERS CONFIGURE_DEPENDS "${MARIADB_SOURCE_INCLUDE_DIR}/*.h" "${MARIADB_BINARY_INCLUDE_DIR}/*.h" )
      file(COPY ${MARIADB_PUBLIC_HEADERS} DESTINATION "${_MARIADB_INCLUDE_DIR}" )

      # Copy MySQL compatibility headers.
      file(GLOB MYSQL_COMPAT_HEADERS CONFIGURE_DEPENDS "${MARIADB_SOURCE_INCLUDE_DIR}/mysql/*.h" )
      file(COPY ${MYSQL_COMPAT_HEADERS} DESTINATION "${_MARIADB_INCLUDE_DIR}/mysql" )

      # Copy MariaDB-specific headers.
      file(GLOB MARIADB_COMPAT_HEADERS CONFIGURE_DEPENDS "${MARIADB_SOURCE_INCLUDE_DIR}/mariadb/*.h" )
      file(COPY ${MARIADB_COMPAT_HEADERS} DESTINATION "${_MARIADB_INCLUDE_DIR}/mariadb" )
      
      # -----------------------------------------------------------
      set(TOBASA_MARIADB_INCLUDE_DIRS "${MARIADB_INCLUDE_DIRS}")
      if(TOBASA_SQL_USE_MARIADB_DLL)
         set(TOBASA_MARIADB_LIBRARIES "libmariadb")    # dynamic lib
      else()
         set(TOBASA_MARIADB_LIBRARIES "mariadbclient") # static library
      endif()
      set(TOBASA_MARIADB_FOUND TRUE)

      restore_cmake_pic_state()
   endif()

   message(STATUS "tobasasql MariaDB include dirs  : ${TOBASA_MARIADB_INCLUDE_DIRS}")
   message(STATUS "tobasasql MariaDB libraries     : ${TOBASA_MARIADB_LIBRARIES}")
   message(STATUS "tobasasql MariaDB DLL           : ${TOBASA_SQL_USE_MARIADB_DLL}")

endmacro()