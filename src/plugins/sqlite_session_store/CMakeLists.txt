cmake_minimum_required(VERSION 3.12-3.18)
cmake_policy(SET CMP0091 NEW)

project(plugin_sqlite_session_store VERSION 1.0.0 LANGUAGES CXX)

if(TOBASA_USE_SQLITE3_MC)
   # SQLite3 Multiple Chipers sources included
   set(TOBASA_SQLITE3_LIB "sqlite3_mc")
else()
   # SQLite3 library via find_package
   find_package(unofficial-sqlite3 CONFIG REQUIRED)
   set(TOBASA_SQLITE3_LIB "unofficial::sqlite3::sqlite3")
endif()

set(PLUGIN_EXE_NAME "session_store_sqlite_plugin")

set(PROJECT_SOURCES
   src/main.h
   src/main.cpp
   src/sqlite_session_store.h
   src/sqlite_session_store.cpp
)

# Create shared library (plugin)
add_library(${PLUGIN_EXE_NAME} SHARED ${PROJECT_SOURCES})

# Set visibility properties
set_target_properties(${PLUGIN_EXE_NAME} PROPERTIES
   CXX_VISIBILITY_PRESET hidden
   VISIBILITY_INLINES_HIDDEN ON )

# Include directories
target_include_directories(${PLUGIN_EXE_NAME}
   PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src )

# Link against Tobasa framework
target_link_libraries(${PLUGIN_EXE_NAME}
   PRIVATE tobasa tobasahttp tobasaweb tobasasql ${TOBASA_SQLITE3_LIB})

if(MSVC)
   target_compile_definitions( ${PLUGIN_EXE_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS )
endif()

if(CMAKE_CXX20_COMPILE_FEATURES)
   target_compile_features(${PLUGIN_EXE_NAME} PUBLIC cxx_std_20)
else()
   target_compile_features(${PLUGIN_EXE_NAME} PUBLIC cxx_std_17)
endif()

# On Windows, ensure proper DLL export
if(MSVC)
   target_compile_options(${PLUGIN_EXE_NAME} PRIVATE /W4 /bigobj /FS)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
   target_compile_options(${PLUGIN_EXE_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()