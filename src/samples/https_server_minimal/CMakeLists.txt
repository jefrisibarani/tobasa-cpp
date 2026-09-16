cmake_minimum_required(VERSION 3.12-3.18)
cmake_policy(SET CMP0091 NEW)

project(https_server_minimal LANGUAGES CXX)

include(${PROJECT_SOURCE_DIR}/cmake/sources.cmake)

function(configure_https_server_target target source_file)

   add_executable(${target} ${PROJECT_SOURCE_DIR}/src/${source_file})

   target_sources( ${target}
      PRIVATE ${MY_HEADERS_LIST}
      PRIVATE $<$<CXX_COMPILER_ID:MSVC>:${PROJECT_SOURCE_DIR}/src/tobasa_app.rc> )

   target_link_libraries( ${target} PRIVATE tobasahttp tobasa)

   target_include_directories( ${target} PRIVATE ${PROJECT_SOURCE_DIR}/src )

   if(MSVC)
      set_property(TARGET ${target} PROPERTY
         MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

      target_compile_definitions( ${target}
         PRIVATE _CRT_SECURE_NO_WARNINGS
         PRIVATE WIN32_LEAN_AND_MEAN )
   else()
      if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
         target_compile_options(${target} PRIVATE -pthread )
         target_link_libraries(${target} PRIVATE pthread)
      endif()
   endif()

   if(CMAKE_CXX20_COMPILE_FEATURES)
      target_compile_features(${target} PRIVATE cxx_std_20)
   else()
      target_compile_features(${target} PRIVATE cxx_std_17)
   endif()

   if(MSVC)
      target_compile_options(${target} PRIVATE /bigobj /FS )
   endif()


   # Pre Build and Post Build commands
   # ------------------------------------------------------
   # Copy TLS certificates
   add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
         ${PROJECT_SOURCE_DIR}/resources/ $<TARGET_FILE_DIR:${target}>/)

   # Copy Timezone database
   # -------------------------------------------------------
   if(NOT TOBASA_USE_STD_CALENDAR_TZONE)
      if (NOT TOBASA_BUILD_IN_MEMORY_TZDB)
         add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${TOBASA_FRAMEWORK_ROOTDIR}/res/tzdata/ $<TARGET_FILE_DIR:${target}>/tzdata/)
      endif()
   endif()

   # Copy binary dir to _output dir
   # -------------------------------------------------------
   if(TOBASA_CREATE_PACKAGE_OUTPUT_DIR)
      set(package_output_dir ${TOBASA_PACKAGE_OUTPUT_DIR}/${PROJECT_NAME}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>)

      add_custom_command(TARGET ${target} POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy_directory
            $<TARGET_FILE_DIR:${target}>/ ${package_output_dir}/ )

      add_custom_command(TARGET ${target} POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E rm -rf ${package_output_dir}/CMakeFiles )

      add_custom_command(TARGET ${target} POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E rm -f ${package_output_dir}/cmake_install.cmake )

      add_custom_command(TARGET ${target} POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E rm -f ${package_output_dir}/Makefile )
   endif()
endfunction()

configure_https_server_target(server_with_worker with_worker_threads.cpp)
configure_https_server_target(server_no_worker without_worker_threads.cpp)