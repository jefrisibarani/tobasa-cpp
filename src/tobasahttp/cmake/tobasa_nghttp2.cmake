macro(tobasa_configure_nghttp2)

   set(NGHTTP2_VERSION_TAG "v1.70.0")

   check_cmake_project("${TOBASA_EXTERNAL_DEPS_DIR}/nghttp2" "${NGHTTP2_VERSION_TAG}" _has_bundled_nghttp2)
   if(_has_bundled_nghttp2)
      message(STATUS "tobasahttp Has bundled nghttp2")
   endif()

   if(TOBASA_HTTP_USE_PREBUILT_NGHTTP2)
      message(STATUS "tobasahttp Using prebuilt nghttp2")

      if(NOT DEFINED TOBASA_HTTP2_DIR)
         set(TOBASA_HTTP2_DIR ${TOBASA_HTTP2_DIR_DEF})
      endif()

      set(TOBASA_NGHTTP2_FOUND        TRUE)
      set(TOBASA_NGHTTP2_INCLUDE_DIRS "${TOBASA_HTTP2_DIR}/include")

      if (MSVC)
         set(TOBASA_NGHTTP2_LIBRARIES optimized ${TOBASA_HTTP2_DIR}/lib/nghttp2.lib debug ${TOBASA_HTTP2_DIR}/debug/lib/nghttp2.lib)
      else()
         set(TOBASA_NGHTTP2_LIBRARIES optimized ${TOBASA_HTTP2_DIR}/lib/libnghttp2.a debug ${TOBASA_HTTP2_DIR}/lib/libnghttp2.a)
      endif()

   elseif(TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG)
      if(TOBASA_BUILD_WITH_VCPKG)
         message(STATUS "tobasasql Finding nghttp2 with find_package VCPKG")

         list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" )
         find_package(NGHTTP2 REQUIRED)

         set(TOBASA_NGHTTP2_INCLUDE_DIRS "${NGHTTP2_INCLUDE_DIR}")
         set(TOBASA_NGHTTP2_LIBRARIES    "${NGHTTP2_LIBRARY}")
         set(TOBASA_NGHTTP2_FOUND        TRUE)

      else()
         message(STATUS "tobasasql Finding nghttp2 with find_package")

         find_package(PkgConfig REQUIRED)
         pkg_check_modules(NGHTTP2 REQUIRED IMPORTED_TARGET libnghttp2)
         set(TOBASA_NGHTTP2_INCLUDE_DIRS "${NGHTTP2_INCLUDE_DIR}")
         set(TOBASA_NGHTTP2_LIBRARIES    "${NGHTTP2_LIBRARY}")  #PkgConfig::NGHTTP2
         set(TOBASA_NGHTTP2_FOUND        TRUE)
      endif()

   else()
      # Build from source
      set_and_save_cmake_pic_state()

      # Build options
      # -----------------------------------------------------------
      set(ENABLE_LIB_ONLY     ON)
      set(BUILD_SHARED_LIBS   OFF)
      set(BUILD_STATIC_LIBS   ON)
      set(ENABLE_DOC          OFF)
      # -----------------------------------------------------------

      if(_has_bundled_nghttp2)
         set(TOBASA_SQL_USE_BUNDLED_NGHTTP2 TRUE)
         message(STATUS "tobasahttp using bundled nghttp2")
         add_subdirectory(${TOBASA_EXTERNAL_DEPS_DIR}/nghttp2 ${CMAKE_BINARY_DIR}/_exts/nghttp2 EXCLUDE_FROM_ALL)
      
      else()
         message(STATUS "tobasahttp Fetching nghttp2")
         include(FetchContent)
         FetchContent_Declare(
            nghttp2
            GIT_REPOSITORY https://github.com/nghttp2/nghttp2.git
            GIT_TAG        "${NGHTTP2_VERSION_TAG}"
            GIT_SHALLOW    TRUE
         )
         FetchContent_MakeAvailable(nghttp2)
      endif()

      set(TOBASA_NGHTTP2_LIBRARIES "nghttp2_static")
      if(WIN32)
         set(TOBASA_NGHTTP2_INCLUDE_DIRS "${nghttp2_SOURCE_DIR}/lib" "${nghttp2_SOURCE_DIR}/lib/includes" "${nghttp2_BINARY_DIR}" "${nghttp2_BINARY_DIR}/lib/includes")
      else()
         set(TOBASA_NGHTTP2_INCLUDE_DIRS "${nghttp2_SOURCE_DIR}" "${nghttp2_BINARY_DIR}")
      endif()

      set(TOBASA_NGHTTP2_FOUND        TRUE)

      restore_cmake_pic_state()
   endif()

   message(STATUS "tobasahttp nghttp2 include dirs : ${TOBASA_NGHTTP2_INCLUDE_DIRS}")
   message(STATUS "tobasahttp nghttp2 libraries    : ${TOBASA_NGHTTP2_LIBRARIES}")

endmacro()