cmake_minimum_required(VERSION 3.12-3.18)

set(TOBASA_MONOLITHIC_COMPONENTS
   tobasa
   tobasahttp
   tobasaweb
   tobasasql
   tobasalis
   tobasadicom)

add_library(tobasa_monolithic SHARED)
target_compile_definitions(tobasa_monolithic PRIVATE
   TBS_SHARED
   TBS_HTTP_SHARED
   TBS_WEB_SHARED
   TBS_SQL_SHARED
   TBS_LIS_SHARED
   TBS_DICOM_SHARED)

foreach(component IN LISTS TOBASA_MONOLITHIC_COMPONENTS)
   add_library(${component} INTERFACE)
   target_link_libraries(${component} INTERFACE tobasa_monolithic)
endforeach()

add_library(tobasa::monolithic ALIAS tobasa_monolithic)
