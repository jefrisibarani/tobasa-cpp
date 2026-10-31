macro(tobasa_configure_zlib)

   set(ZLIB_VERSION_TAG "v1.3.2")

   check_cmake_project("${TOBASA_EXTERNAL_DEPS_DIR}/zlib" "${ZLIB_VERSION_TAG}" _has_bundled_zlib)
   if(_has_bundled_zlib)
      message(STATUS "tobasa Has bundled Zlib")
   endif()

   if(TOBASA_USE_PREBUILT_ZLIB)
      message(STATUS "tobasa Using prebuilt Zlib")

      set(TOBASA_ZLIB_INCLUDE_DIRS ${TOBASA_ZLIB_DIR}/include)
      set(TOBASA_ZLIB_LIBRARIES    optimized ${TOBASA_ZLIB_DIR}/lib/zlib.lib debug ${TOBASA_ZLIB_DIR}/debug/lib/zlibd.lib )
      set(TOBASA_ZLIB_FOUND        TRUE)
      set(ZLIB_FOUND               TRUE)
      set(ZLIB_INCLUDE_DIRS        "${TOBASA_ZLIB_INCLUDE_DIRS}")
      set(ZLIB_LIBRARIES           "${TOBASA_ZLIB_LIBRARIES}")
   
   elseif(TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG)
      if(TOBASA_BUILD_WITH_VCPKG)
         message(STATUS "tobasa Finding Zlib with find_package VCPKG")
      else()
         message(STATUS "tobasa Finding Zlib with find_package")
      endif()

      set(ZLIB_USE_STATIC_LIBS ON)
      find_package(ZLIB REQUIRED)
      set(TOBASA_ZLIB_INCLUDE_DIRS "${ZLIB_INCLUDE_DIRS}")
      set(TOBASA_ZLIB_LIBRARIES    "${ZLIB_LIBRARIES}")
      set(TOBASA_ZLIB_FOUND TRUE)

   else()
      # Build from source
      set_and_save_cmake_pic_state()

      # Build options
      # -----------------------------------------------------------
      set(ZLIB_BUILD_STATIC    ON  CACHE BOOL "" FORCE)
      set(ZLIB_BUILD_SHARED    OFF CACHE BOOL "" FORCE)
      set(ZLIB_BUILD_TESTING   OFF CACHE BOOL "" FORCE)
      set(ZLIB_BUILD_MINIZIP   ON  CACHE BOOL "" FORCE)
      set(ZLIB_INSTALL         OFF CACHE BOOL "" FORCE)
      set(MINIZIP_ENABLE_BZIP2 OFF CACHE BOOL "" FORCE)
      # -----------------------------------------------------------

      if(_has_bundled_zlib)
         set(TOBASA_USE_BUNDLED_ZLIB ON)
         message(STATUS "tobasa Using bundled Zlib")
         add_subdirectory(${TOBASA_EXTERNAL_DEPS_DIR}/zlib ${CMAKE_BINARY_DIR}/_exts/zlib EXCLUDE_FROM_ALL)

      else()
         message(STATUS "tobasa Fetching Zlib")
         include(FetchContent)
         FetchContent_Declare(
            zlib
            GIT_REPOSITORY https://github.com/madler/zlib.git
            GIT_TAG        "${ZLIB_VERSION_TAG}"
            GIT_SHALLOW    TRUE
         )
         FetchContent_MakeAvailable(zlib)
      endif()

      set(ZLIB_FOUND               TRUE)
      set(ZLIB_LIBRARIES           "ZLIB::ZLIBSTATIC")
      set(ZLIB_INCLUDE_DIRS        "${zlib_SOURCE_DIR}" "${zlib_BINARY_DIR}")
      set(MINIZIP_LIBRARIES        "MINIZIP::minizipstatic")
      set(MINIZIP_INCLUDE_DIRS     "${zlib_SOURCE_DIR}/contrib/minizip")

      set(TOBASA_ZLIB_INCLUDE_DIRS "${ZLIB_INCLUDE_DIRS}")
      set(TOBASA_ZLIB_LIBRARIES    "${ZLIB_LIBRARIES}")
      set(TOBASA_ZLIB_FOUND        ${ZLIB_FOUND})

      restore_cmake_pic_state()
   endif()

   message(STATUS "tobasa Zlib include dirs        : ${TOBASA_ZLIB_INCLUDE_DIRS}")
   message(STATUS "tobasa Zlib libraries           : ${TOBASA_ZLIB_LIBRARIES}")

endmacro()