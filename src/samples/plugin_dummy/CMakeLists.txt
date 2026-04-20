cmake_minimum_required(VERSION 3.12-3.18)
cmake_policy(SET CMP0091 NEW)

project(plugin_dummy VERSION 1.0.0 LANGUAGES CXX)

set(PROJECT_SOURCES
   src/dummy.h
   src/dummy.cpp
)

# Create shared library (plugin)
add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCES})

# Set visibility properties
set_target_properties(${PROJECT_NAME} PROPERTIES
   CXX_VISIBILITY_PRESET hidden
   VISIBILITY_INLINES_HIDDEN ON )

# Include directories
target_include_directories(${PROJECT_NAME}
   PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src )

# Link against Tobasa framework
target_link_libraries(${PROJECT_NAME}
   PRIVATE tobasa )

if(MSVC)
   set_property(TARGET ${PROJECT_NAME} PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

   target_compile_definitions( ${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS )
endif()

if(CMAKE_CXX20_COMPILE_FEATURES)
   target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
else()
   target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
endif()

# On Windows, ensure proper DLL export
if(MSVC)
   target_compile_options(${PROJECT_NAME} PRIVATE /W4 /bigobj /FS)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
   target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()


# Output directory
#set_target_properties(${PROJECT_NAME} PROPERTIES
#   LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
#   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"  )

# Copy binary to _output dir
# -------------------------------------------------------
if(TOBASA_CREATE_PACKAGE_OUTPUT_DIR)

   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory
         "${TOBASA_PACKAGE_OUTPUT_DIR}/${PROJECT_NAME}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>"
      COMMAND ${CMAKE_COMMAND} -E copy
         "$<TARGET_FILE:${PROJECT_NAME}>"
         "${TOBASA_PACKAGE_OUTPUT_DIR}/${PROJECT_NAME}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>/"
   )

endif()

