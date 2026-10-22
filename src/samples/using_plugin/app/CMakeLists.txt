cmake_minimum_required(VERSION 3.12-3.18)
cmake_policy(SET CMP0091 NEW)

project(app_using_plugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(APP_EXE_NAME "using_plugin")
if(TOBASA_BUILD_MONOLITHIC_SHARED_LIB)
   set(TOBASA_RUNTIME_LIBRARY_TARGET tobasa_monolithic)
else()
   set(TOBASA_RUNTIME_LIBRARY_TARGET tobasa)
endif()

add_executable(${APP_EXE_NAME})
target_sources( ${APP_EXE_NAME} PRIVATE src/main.cpp )

target_include_directories(${APP_EXE_NAME} PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(${APP_EXE_NAME} PRIVATE tobasa)
tobasa_stage_shared_runtime(${APP_EXE_NAME} tobasa)

# Platform-specific linking
if(UNIX AND NOT APPLE)
   # Linux: need -ldl for dlopen/dlsym
   target_link_libraries(${APP_EXE_NAME} PRIVATE dl)
endif()

# Windows: user32.lib is optional, but used for demo
if(WIN32)
   target_link_libraries(${APP_EXE_NAME} PRIVATE user32)
endif()

# macOS: nothing extra required (dl is in libc)

if(CMAKE_CXX20_COMPILE_FEATURES)
   target_compile_features(${APP_EXE_NAME} PRIVATE cxx_std_20)
else()
   target_compile_features(${APP_EXE_NAME} PRIVATE cxx_std_17)
endif()


add_custom_command(TARGET ${APP_EXE_NAME} POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${APP_EXE_NAME}>/plugins"
   COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:sample_plugin_dummy>" "$<TARGET_FILE_DIR:${APP_EXE_NAME}>/plugins/"
   COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:sample_plugin_hello>" "$<TARGET_FILE_DIR:${APP_EXE_NAME}>/plugins/"
)

# Copy binary to _output dir
# -------------------------------------------------------
if(TOBASA_CREATE_PACKAGE_OUTPUT_DIR)
   add_custom_command(TARGET ${APP_EXE_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory
         "${TOBASA_PACKAGE_OUTPUT_DIR}/${APP_EXE_NAME}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>"
      COMMAND ${CMAKE_COMMAND} -E copy
         "$<TARGET_FILE:${APP_EXE_NAME}>"
         "${TOBASA_PACKAGE_OUTPUT_DIR}/${APP_EXE_NAME}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>/"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
         "$<TARGET_FILE:${TOBASA_RUNTIME_LIBRARY_TARGET}>"
         "${TOBASA_PACKAGE_OUTPUT_DIR}/${APP_EXE_NAME}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>/"
      COMMAND ${CMAKE_COMMAND} -E copy_directory
         "$<TARGET_FILE_DIR:${APP_EXE_NAME}>/plugins"
         "${TOBASA_PACKAGE_OUTPUT_DIR}/${APP_EXE_NAME}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>/plugins"
   )
endif()