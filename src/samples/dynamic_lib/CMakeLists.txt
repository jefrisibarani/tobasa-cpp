cmake_minimum_required(VERSION 3.12-3.18)
cmake_policy(SET CMP0091 NEW)

project(dyn_lib_sample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME})
target_sources( ${PROJECT_NAME} PRIVATE main.cpp )

if(MSVC)
   set_property(TARGET ${PROJECT_NAME} PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE tobasa)

# Platform-specific linking
if(UNIX AND NOT APPLE)
   # Linux: need -ldl for dlopen/dlsym
   target_link_libraries(${PROJECT_NAME} PRIVATE dl)
endif()

# Windows: user32.lib is optional, but used for demo
if(WIN32)
   target_link_libraries(${PROJECT_NAME} PRIVATE user32)
endif()

# macOS: nothing extra required (dl is in libc)

if(CMAKE_CXX20_COMPILE_FEATURES)
   message(STATUS "${PROJECT_NAME} use cxx_std_20")
   target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
else()
   message(STATUS "${PROJECT_NAME} use cxx_std_17")
   target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
endif()


# Copy binary dir to _output dir
# -------------------------------------------------------
if(TOBASA_CREATE_PACKAGE_OUTPUT_DIR)
   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
         $<TARGET_FILE_DIR:${PROJECT_NAME}>/ ${TOBASA_PACKAGE_OUTPUT_DIR}/${PROJECT_NAME}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>/ )

   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E rm -rf ${TOBASA_PACKAGE_OUTPUT_DIR}/${PROJECT_NAME}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>/CMakeFiles )

   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E rm -f ${TOBASA_PACKAGE_OUTPUT_DIR}/${PROJECT_NAME}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>/cmake_install.cmake )
      
   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E rm -f ${TOBASA_PACKAGE_OUTPUT_DIR}/${PROJECT_NAME}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>/Makefile )
endif()