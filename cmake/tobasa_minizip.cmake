macro(tobasa_configure_minizip)

   if(TOBASA_BUILD_ZLIB_FROM_SOURCE)
      if(TARGET MINIZIP::minizipstatic)
         set(TOBASA_MINIZIP_LIBRARIES "MINIZIP::minizipstatic")
      elseif(TARGET MINIZIP::minizip)
         set(TOBASA_MINIZIP_LIBRARIES "MINIZIP::minizip")
      else()
         message(FATAL_ERROR
            "Zlib was built from source, but no minizip target is available")
      endif()

      if(DEFINED zlib_SOURCE_DIR)
         set(TOBASA_MINIZIP_INCLUDE_DIRS "${zlib_SOURCE_DIR}/contrib")
      else()
         set(TOBASA_MINIZIP_INCLUDE_DIRS "${TOBASA_EXTERNAL_DEPS_DIR}/zlib/contrib")
      endif()
      set(TOBASA_MINIZIP_FOUND TRUE)

   elseif(TOBASA_USE_PREBUILT_MINIZIP)
      find_path(TOBASA_MINIZIP_INCLUDE_DIRS
         NAMES minizip/zip.h zip.h
         PATHS "${TOBASA_MINIZIP_DIR}/include"
         NO_DEFAULT_PATH)

      find_library(TOBASA_MINIZIP_LIBRARY_RELEASE
         NAMES minizips minizip
         PATHS "${TOBASA_MINIZIP_DIR}/lib"
         NO_DEFAULT_PATH)
      find_library(TOBASA_MINIZIP_LIBRARY_DEBUG
         NAMES minizipsd minizipd minizips minizip
         PATHS "${TOBASA_MINIZIP_DIR}/debug/lib" "${TOBASA_MINIZIP_DIR}/lib"
         NO_DEFAULT_PATH)

      if(NOT TOBASA_MINIZIP_INCLUDE_DIRS OR
         NOT TOBASA_MINIZIP_LIBRARY_RELEASE OR
         NOT TOBASA_MINIZIP_LIBRARY_DEBUG)
         message(FATAL_ERROR
            "Could not find prebuilt minizip under ${TOBASA_MINIZIP_DIR}")
      endif()

      set(TOBASA_MINIZIP_LIBRARIES
         optimized "${TOBASA_MINIZIP_LIBRARY_RELEASE}"
         debug "${TOBASA_MINIZIP_LIBRARY_DEBUG}")
      set(TOBASA_MINIZIP_FOUND TRUE)

   elseif(TARGET MINIZIP::minizipstatic)
      set(TOBASA_MINIZIP_LIBRARIES "MINIZIP::minizipstatic")
      set(TOBASA_MINIZIP_INCLUDE_DIRS "${TOBASA_EXTERNAL_DEPS_DIR}/zlib/contrib/minizip")
      set(TOBASA_MINIZIP_FOUND TRUE)

   elseif(TARGET MINIZIP::minizip)
      set(TOBASA_MINIZIP_LIBRARIES "MINIZIP::minizip")
      set(TOBASA_MINIZIP_INCLUDE_DIRS "${TOBASA_EXTERNAL_DEPS_DIR}/zlib/contrib/minizip")
      set(TOBASA_MINIZIP_FOUND TRUE)

   elseif(TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG)
      if(TOBASA_FIND_DEPS)
         find_package(minizip CONFIG REQUIRED)
      else()
         find_package(unofficial-minizip CONFIG REQUIRED)
      endif()


      if(TARGET MINIZIP::minizipstatic)
         set(TOBASA_MINIZIP_LIBRARIES "MINIZIP::minizipstatic")
      elseif(TARGET MINIZIP::minizip)
         set(TOBASA_MINIZIP_LIBRARIES "MINIZIP::minizip")
      elseif(TARGET minizip::minizipstatic)
         set(TOBASA_MINIZIP_LIBRARIES "minizip::minizipstatic")
      elseif(TARGET minizip::minizip)
         set(TOBASA_MINIZIP_LIBRARIES "minizip::minizip")
      elseif(TARGET unofficial::minizip::minizip)
         set(TOBASA_MINIZIP_LIBRARIES "unofficial::minizip::minizip")
      else()
         message(FATAL_ERROR "minizip was found, but it does not provide a supported CMake target")
      endif()

      if(DEFINED minizip_INCLUDE_DIRS)
         set(TOBASA_MINIZIP_INCLUDE_DIRS "${minizip_INCLUDE_DIRS}")
      endif()
      
      set(TOBASA_MINIZIP_FOUND TRUE)

   else()
      message(FATAL_ERROR
         "Minizip is unavailable: configure zlib before configuring minizip")
   endif()

   message(STATUS "tobasa Minizip include dirs     : ${TOBASA_MINIZIP_INCLUDE_DIRS}")
   message(STATUS "tobasa Minizip libraries        : ${TOBASA_MINIZIP_LIBRARIES}")

endmacro()
