cmake_minimum_required(VERSION 3.12-3.18)
cmake_policy(SET CMP0091 NEW)

option(TOBASA_BUILD_IN_MEMORY_RESOURCES "Whether to use compiled resources" OFF)

project(test_tobasa LANGUAGES CXX)

if(TOBASA_BUILD_IN_MEMORY_RESOURCES)
   add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/views_resources.cpp"
      COMMAND generate_resources_file ${PROJECT_SOURCE_DIR}/views views ${CMAKE_CURRENT_BINARY_DIR}/views_resources.cpp getViewsResources appview
      DEPENDS generate_resources_file
   )
endif()

add_executable(${PROJECT_NAME})
target_sources( ${PROJECT_NAME} PRIVATE main.cpp )

if(TOBASA_BUILD_IN_MEMORY_RESOURCES)
   target_sources( ${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/views_resources.cpp )
   target_compile_definitions( ${PROJECT_NAME} PRIVATE TOBASA_BUILD_IN_MEMORY_RESOURCES )
endif()

if(MSVC)
   set_property(TARGET ${PROJECT_NAME} PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE tobasa)

if(CMAKE_CXX20_COMPILE_FEATURES)
   message(STATUS "${PROJECT_NAME} use cxx_std_20")
   target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
else()
   message(STATUS "${PROJECT_NAME} use cxx_std_17")
   target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
endif()


# Copy Timezone database
# -------------------------------------------------------
if(NOT TOBASA_USE_STD_CALENDAR_TZONE)
   if (NOT TOBASA_BUILD_IN_MEMORY_TZDB)
      add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy_directory
         ${TOBASA_FRAMEWORK_ROOTDIR}/res/tzdata/ $<TARGET_FILE_DIR:${PROJECT_NAME}>/tzdata/)
   endif()
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