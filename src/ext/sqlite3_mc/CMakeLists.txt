cmake_minimum_required(VERSION 3.12-3.18)
cmake_policy(SET CMP0091 NEW)

project(sqlite3_mc C)

set(CODEC_TYPE "CODEC_TYPE_SQLCIPHER")
# WE use $<BOOL:$<$<CONFIG:Debug>:DEBUG_BUILD>>
#set(SQLITE_ENABLE_DEBUG 0)

add_library(${PROJECT_NAME})

if(WIN32)
   set_property(TARGET ${PROJECT_NAME} PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

target_sources(${PROJECT_NAME}
   PRIVATE ${PROJECT_SOURCE_DIR}/src/sqlite3mc.c
)

target_include_directories(${PROJECT_NAME}
   PUBLIC ${PROJECT_SOURCE_DIR}/src
)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
   target_compile_options(${PROJECT_NAME} PRIVATE -msse4.2 -maes )
   target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl m )
endif()
#Target compile definitions
#-------------------------------------------------------------------------------
if(MSVC)
   target_compile_definitions(${PROJECT_NAME}
      PUBLIC _CRT_SECURE_NO_WARNINGS
             _CRT_SECURE_NO_DEPRECATE
             _CRT_NONSTDC_NO_WARNINGS
             _CRT_NONSTDC_NO_DEPRECATE)
endif()

target_compile_definitions(${PROJECT_NAME}
   PUBLIC CODEC_TYPE=${CODEC_TYPE}
          SQLITE_ENABLE_DEBUG=$<BOOL:$<$<CONFIG:Debug>:DEBUG_BUILD>>
          SQLITE_THREADSAFE=1
          SQLITE_DQS=0
          SQLITE_MAX_ATTACHED=10
          SQLITE_SOUNDEX=1
          SQLITE_ENABLE_COLUMN_METADATA=1
          SQLITE_SECURE_DELETE=1
          SQLITE_ENABLE_DESERIALIZE=1
          SQLITE_ENABLE_FTS3=1
          SQLITE_ENABLE_FTS3_PARENTHESIS=1
          SQLITE_ENABLE_FTS4=1
          SQLITE_ENABLE_FTS5=1
          SQLITE_ENABLE_JSON1=1
          SQLITE_ENABLE_RTREE=1
          SQLITE_ENABLE_GEOPOLY=1
          SQLITE_ENABLE_PREUPDATE_HOOK=1
          SQLITE_CORE=1
          SQLITE_ENABLE_EXTFUNC=1
          SQLITE_ENABLE_MATH_FUNCTIONS=1
          SQLITE_ENABLE_CSV=1
          SQLITE_ENABLE_VSV=1
          SQLITE_ENABLE_SHA3=1
          SQLITE_ENABLE_CARRAY=1
          SQLITE_ENABLE_FILEIO=1
          SQLITE_ENABLE_SERIES=1
          SQLITE_ENABLE_UUID=1
          SQLITE_ENABLE_REGEXP=1
          SQLITE_TEMP_STORE=2
          SQLITE_USE_URI=1
          SQLITE_OMIT_SHARED_CACHE=1
          SQLITE_USER_AUTHENTICATION=0
          SQLITE3MC_USE_MINIZ=0
          TOBASA_SQL_USE_SQLITE3_MC)

file(GLOB SQLITE3_MC_HEADERS_LIST CONFIGURE_DEPENDS
   ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

# IDEs should put the headers in a nice place
source_group(
  TREE "${PROJECT_SOURCE_DIR}/src"
  PREFIX "Header Files"
  FILES ${SQLITE3_MC_HEADERS_LIST} )
