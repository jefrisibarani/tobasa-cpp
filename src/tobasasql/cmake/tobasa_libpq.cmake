macro(tobasa_configure_libpq)

   if(TOBASA_SQL_USE_PREBUILT_LIBPQ)
      message(STATUS "tobasasql Using prebuilt Libpq")

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

      # Link with statically built libpq
      set(TOBASA_LIBPQ_LIBRARIES wldap32.lib 
         optimized ${TOBASA_LIBPQ_DIR}/lib/libpq.lib       debug ${TOBASA_LIBPQ_DIR}/debug/lib/libpq.lib 
         optimized ${TOBASA_LIBPQ_DIR}/lib/libpgport.lib   debug ${TOBASA_LIBPQ_DIR}/debug/lib/libpgport.lib
         optimized ${TOBASA_LIBPQ_DIR}/lib/libpgcommon.lib debug ${TOBASA_LIBPQ_DIR}/debug/lib/libpgcommon.lib
         optimized ${TOBASA_OPENSSL_DIR}/lib/libssl.lib    debug ${TOBASA_OPENSSL_DIR}/debug/lib/libssl.lib
         optimized ${TOBASA_OPENSSL_DIR}/lib/libcrypto.lib debug ${TOBASA_OPENSSL_DIR}/debug/lib/libcrypto.lib
      )

      set(TOBASA_LIBPQ_INCLUDE_DIRS ${TOBASA_LIBPQ_DIR}/include)
      set(TOBASA_LIBPQ_LIBRARY_DIRS ${TOBASA_LIBPQ_DIR}/lib)
      set(TOBASA_PGSQL_FOUND        TRUE)

   elseif(TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")

      if(TOBASA_BUILD_WITH_VCPKG)
         message(STATUS "tobasasql Finding Libpq with find_package VCPKG")

         find_package(PostgreSQL REQUIRED)
         set(TOBASA_LIBPQ_INCLUDE_DIRS "${PostgreSQL_INCLUDE_DIRS}")
         set(TOBASA_LIBPQ_LIBRARY_DIRS "${PostgreSQL_LIBRARY_DIRS}")
         set(TOBASA_LIBPQ_LIBRARIES    "PostgreSQL::PostgreSQL")
         set(TOBASA_PGSQL_FOUND        TRUE)
      else()
         message(STATUS "tobasasql Finding Libpq with find_package")

         find_package(PostgreSQL REQUIRED)
         set(TOBASA_LIBPQ_INCLUDE_DIRS "${PostgreSQL_INCLUDE_DIRS}")
         set(TOBASA_LIBPQ_LIBRARY_DIRS "${PostgreSQL_LIBRARY_DIRS}")
         set(TOBASA_LIBPQ_LIBRARIES    "PostgreSQL::PostgreSQL")
         set(TOBASA_PGSQL_FOUND        TRUE)
      endif()

   endif()

   message(STATUS "tobasasql Libpq include dirs : ${TOBASA_LIBPQ_INCLUDE_DIRS}")
   message(STATUS "tobasasql Libpq library dirs : ${TOBASA_LIBPQ_LIBRARY_DIRS}")
   message(STATUS "tobasasql Libpq libraries    : ${TOBASA_LIBPQ_LIBRARIES}")

endmacro()