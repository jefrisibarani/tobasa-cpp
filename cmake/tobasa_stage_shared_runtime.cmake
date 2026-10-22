# Copies the specified shared-library files to the destination target's output directory.
# The first parameter is the destination target; remaining parameters are source targets.
# Only runs on Windows when TOBASA_BUILD_SHARED_LIB is enabled.
#
# Usage:
#   tobasa_stage_shared_runtime(myapp tobasa tobasaweb)
set(TOBASA_RUNTIME_COPY_SCRIPT
   "${CMAKE_CURRENT_LIST_DIR}/tobasa_copy_runtime_dependencies.cmake")

function(tobasa_stage_shared_runtime target)
   if(WIN32 AND TOBASA_BUILD_SHARED_LIB)
      set(copy_commands)
      set(runtime_search_dirs
         "${CMAKE_BINARY_DIR}/bin"
         "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

      if(TOBASA_BUILD_MONOLITHIC_SHARED_LIB)
         list(APPEND copy_commands
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
               "$<TARGET_FILE:tobasa_monolithic>"
               "$<TARGET_FILE_DIR:${target}>")
         list(APPEND runtime_search_dirs "$<TARGET_FILE_DIR:tobasa_monolithic>")
      else()
         foreach(runtime_target IN LISTS ARGN)
            if(TARGET ${runtime_target})
               get_target_property(runtime_target_type ${runtime_target} TYPE)
               if(NOT runtime_target_type STREQUAL "INTERFACE_LIBRARY")
                  list(APPEND copy_commands
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "$<TARGET_FILE:${runtime_target}>"
                        "$<TARGET_FILE_DIR:${target}>")
                  list(APPEND runtime_search_dirs "$<TARGET_FILE_DIR:${runtime_target}>")
               endif()
            endif()
         endforeach()
      endif()

      if(NOT TOBASA_BUILD_MONOLITHIC_SHARED_LIB)
         foreach(tobasa_target IN ITEMS
            tobasa tobasahttp tobasasql tobasaweb tobasalis tobasadicom)
            if(TARGET ${tobasa_target})
               get_target_property(tobasa_target_type ${tobasa_target} TYPE)
               if(NOT tobasa_target_type STREQUAL "INTERFACE_LIBRARY")
                  list(APPEND runtime_search_dirs "$<TARGET_FILE_DIR:${tobasa_target}>")
                  if(CMAKE_VERSION VERSION_LESS 3.16)
                     list(APPEND copy_commands
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           "$<TARGET_FILE:${tobasa_target}>"
                           "$<TARGET_FILE_DIR:${target}>")
                  endif()
               endif()
            endif()
         endforeach()
      endif()

      foreach(dependency_root_var IN ITEMS
         TOBASA_OPENSSL_DIR
         TOBASA_LIBPQ_DIR
         TOBASA_ZLIB_DIR
         TOBASA_MINIZIP_DIR
         TOBASA_PROTOBUF_DIR
         TOBASA_HTTP2_DIR
         TOBASA_LIBPNG_DIR
         TOBASA_LIBXML2_DIR
         TOBASA_OPENJPEG_DIR
         TOBASA_LIBTIFF_DIR)
         if(DEFINED ${dependency_root_var} AND NOT "${${dependency_root_var}}" STREQUAL "")
            list(APPEND runtime_search_dirs
               "${${dependency_root_var}}/bin"
               "${${dependency_root_var}}/debug/bin"
               "${${dependency_root_var}}/lib"
               "${${dependency_root_var}}/debug/lib")
         endif()
      endforeach()

      if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
         list(APPEND runtime_search_dirs
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin"
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin")
      endif()

      foreach(package_prefix IN LISTS CMAKE_PREFIX_PATH)
         list(APPEND runtime_search_dirs
            "${package_prefix}/bin"
            "${package_prefix}/debug/bin"
            "${package_prefix}/lib"
            "${package_prefix}/debug/lib")
      endforeach()

      list(REMOVE_DUPLICATES runtime_search_dirs)

      if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.16)
         string(JOIN "|" runtime_search_dirs_arg ${runtime_search_dirs})
         list(APPEND copy_commands
            COMMAND ${CMAKE_COMMAND}
               "-DTOBASA_RUNTIME_EXECUTABLE=$<TARGET_FILE:${target}>"
               "-DTOBASA_RUNTIME_DESTINATION=$<TARGET_FILE_DIR:${target}>"
               "-DTOBASA_RUNTIME_SEARCH_DIRS=${runtime_search_dirs_arg}"
               -P "${TOBASA_RUNTIME_COPY_SCRIPT}")
      else()
         file(GLOB build_runtime_dlls
            "${CMAKE_BINARY_DIR}/bin/*.dll"
            "${CMAKE_BINARY_DIR}/bin/*/*.dll")
         foreach(runtime_dll IN LISTS build_runtime_dlls)
            list(APPEND copy_commands
               COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  "${runtime_dll}"
                  "$<TARGET_FILE_DIR:${target}>")
         endforeach()

         foreach(runtime_search_dir IN LISTS runtime_search_dirs)
            file(GLOB runtime_dlls "${runtime_search_dir}/*.dll")
            foreach(runtime_dll IN LISTS runtime_dlls)
               list(APPEND copy_commands
                  COMMAND ${CMAKE_COMMAND} -E copy_if_different
                     "${runtime_dll}"
                     "$<TARGET_FILE_DIR:${target}>")
            endforeach()
         endforeach()
      endif()

      if(copy_commands)
         add_custom_command(TARGET ${target} POST_BUILD
            ${copy_commands}
            VERBATIM)
      endif()
   endif()
endfunction()