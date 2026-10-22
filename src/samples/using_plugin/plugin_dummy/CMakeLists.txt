cmake_minimum_required(VERSION 3.12-3.18)
cmake_policy(SET CMP0091 NEW)

project(plugin_dummy VERSION 1.0.0 LANGUAGES CXX)

set(PROJECT_SOURCES
   src/dummy.h
   src/dummy.cpp
)

# Create shared library (plugin)
add_library(sample_plugin_dummy SHARED ${PROJECT_SOURCES})

# Set visibility properties
set_target_properties(sample_plugin_dummy PROPERTIES
   OUTPUT_NAME "plugin_dummy"
   PREFIX ""
   IMPORT_PREFIX ""
   CXX_VISIBILITY_PRESET hidden
   VISIBILITY_INLINES_HIDDEN ON )

# Include directories
target_include_directories(sample_plugin_dummy
   PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src )

# Link against Tobasa framework
target_link_libraries(sample_plugin_dummy
   PRIVATE tobasa )

if(MSVC)
   target_compile_definitions( sample_plugin_dummy PRIVATE _CRT_SECURE_NO_WARNINGS )
endif()

if(CMAKE_CXX20_COMPILE_FEATURES)
   target_compile_features(sample_plugin_dummy PUBLIC cxx_std_20)
else()
   target_compile_features(sample_plugin_dummy PUBLIC cxx_std_17)
endif()

# On Windows, ensure proper DLL export
if(MSVC)
   target_compile_options(sample_plugin_dummy PRIVATE /W4 /bigobj /FS)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
   target_compile_options(sample_plugin_dummy PRIVATE -Wall -Wextra -Wpedantic)
endif()