macro(tobasa_configure_sqlite)

   if((TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG) AND NOT TOBASA_USE_BUNDLED_SQLITE3_MC)
      set(TOBASA_USE_SQLITE3_MC OFF)
   else()
      set(TOBASA_USE_SQLITE3_MC ON)
   endif()

   if(TOBASA_USE_SQLITE3_MC)
      # SQLite3 Multiple Chipers sources included
      set(TOBASA_SQLITE3_LIB "sqlite3_mc")
      add_subdirectory(${TOBASA_EXTERNAL_DEPS_DIR}/sqlite3_mc ${CMAKE_BINARY_DIR}/_exts/sqlite3_mc EXCLUDE_FROM_ALL)
   else()
      # SQLite3 library via find_package
      find_package(unofficial-sqlite3 CONFIG REQUIRED)
      set(TOBASA_SQLITE3_LIB "unofficial::sqlite3::sqlite3")
   endif()
   message(STATUS "tobasasql sqlite3 lib: ${TOBASA_SQLITE3_LIB}")

endmacro()