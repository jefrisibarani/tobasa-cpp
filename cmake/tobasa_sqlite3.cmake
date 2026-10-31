macro(tobasa_configure_sqlite3)

   set(SQLITE3MC_VERSION_TAG "v2.6.1")

   check_cmake_project("${TOBASA_EXTERNAL_DEPS_DIR}/sqlite3mc" "${SQLITE3MC_VERSION_TAG}" _has_bundled_sqlite3mc)
   if(_has_bundled_sqlite3mc)
      message(STATUS "tobasa Has bundled SQLite3MC")
   endif()

   if(TOBASA_USE_PREBUILT_SQLITE3)
      message(STATUS "tobasa Using prebuilt SQLite3")
      
      set(TOBASA_USE_SQLITE3_MC OFF)
      set(TOBASA_SQLITE3_LIBRARIES  optimized ${TOBASA_SQLITE3_DIR}/lib/sqlite3.lib debug ${TOBASA_SQLITE3_DIR}/debug/lib/sqlite3.lib )
      set(TOBASA_SQLITE3_INCLUDE_DIRS ${TOBASA_SQLITE3_DIR}/include)
      set(TOBASA_SQLITE3_FOUND TRUE)

   elseif(TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG)
      set(TOBASA_USE_SQLITE3_MC OFF)

      if(TOBASA_BUILD_WITH_VCPKG)
         message(STATUS "tobasa Finding SQLite3 with find_package VCPKG")
         find_package(unofficial-sqlite3 CONFIG REQUIRED)
         set(TOBASA_SQLITE3_LIBRARIES "unofficial::sqlite3::sqlite3")
         set(TOBASA_SQLITE3_INCLUDE_DIRS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include")
         set(TOBASA_SQLITE3_FOUND $unofficial-sqlite3_FOUND)

      else()
         message(STATUS "tobasa Finding SQLite3 with find_package")
         find_package(SQLite3 CONFIG REQUIRED)
         set(TOBASA_SQLITE3_INCLUDE_DIRS "${SQLite3_INCLUDE_DIRS}")
         set(TOBASA_SQLITE3_LIBRARIES "${SQLite3_LIBRARIES}")
         set(TOBASA_SQLITE3_FOUND TRUE)
      endif()

   else()
      # Build from source
      # -----------------------------------------------------------
      set(TOBASA_USE_SQLITE3_MC ON)

      set_and_save_cmake_pic_state()

      # Build options
      # -----------------------------------------------------------
      if(TOBASA_BUILD_SHARED_LIB AND NOT TOBASA_BUILD_MONOLITHIC_SHARED_LIB)
         set(SQLITE3MC_STATIC OFF CACHE BOOL "" FORCE)
         set(_SQLITE3_MC_LIB "sqlite3mc")
      else()
         set(SQLITE3MC_STATIC ON CACHE BOOL "" FORCE)
         set(_SQLITE3_MC_LIB "sqlite3mc_static")
      endif()
      set(CODEC_TYPE            "SQLCIPHER" CACHE STRING "" FORCE)
      set(SQLITE3MC_BUILD_SHELL OFF CACHE BOOL "" FORCE)
      set(SQLITE3MC_USE_MINIZ   OFF CACHE BOOL "" FORCE)
      # -----------------------------------------------------------

      if(_has_bundled_sqlite3mc)
         message(STATUS "tobasa Using bundled SQLite3MC")
         set(TOBASA_USE_BUNDLED_SQLITE3_MC TRUE)
         set(SQLITE3MC_SOURCE_DIR "${TOBASA_EXTERNAL_DEPS_DIR}/sqlite3mc")
         # patch original CMakeLists.txt
         include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/apply_sqlite3mc_patch.cmake")
         add_subdirectory(${TOBASA_EXTERNAL_DEPS_DIR}/sqlite3mc ${CMAKE_BINARY_DIR}/_exts/sqlite3mc EXCLUDE_FROM_ALL)
      else()
         message(STATUS "tobasa Fetching SQLite3MC")
         include(FetchContent)
         FetchContent_Declare(
            sqlite3mc
            GIT_REPOSITORY https://github.com/utelle/SQLite3MultipleCiphers.git
            GIT_TAG        "${SQLITE3MC_VERSION_TAG}"
            GIT_SHALLOW    TRUE
         )
         #FetchContent_MakeAvailable(sqlite3mc)
         # Download without configuring SQLite3MC.
         FetchContent_GetProperties(sqlite3mc)

         if(NOT sqlite3mc_POPULATED)
            FetchContent_Populate(sqlite3mc)
         endif()

         set(SQLITE3MC_SOURCE_DIR "${sqlite3mc_SOURCE_DIR}")
         # patch original CMakeLists.txt
         include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/apply_sqlite3mc_patch.cmake")
         add_subdirectory("${sqlite3mc_SOURCE_DIR}" "${sqlite3mc_BINARY_DIR}")
      endif()

      set(TOBASA_SQLITE3_FOUND TRUE)
      set(TOBASA_SQLITE3_LIBRARIES "${_SQLITE3_MC_LIB}")
      set(TOBASA_SQLITE3_INCLUDE_DIRS "${sqlite3mc_SOURCE_DIR}/src")

      restore_cmake_pic_state()

   endif()

   message(STATUS "tobasa SQLite3 include dirs     : ${TOBASA_SQLITE3_INCLUDE_DIRS}")
   message(STATUS "tobasa SQLite3 libraries        : ${TOBASA_SQLITE3_LIBRARIES}")

endmacro()