cmake_minimum_required(VERSION 3.12-3.18)
cmake_policy(SET CMP0091 NEW)

project(test_tobasasql LANGUAGES CXX)
add_executable(${PROJECT_NAME} simple.cpp)

set(CONNECTOR_TARGET test_tobasasql_connector)
add_executable(${CONNECTOR_TARGET} connector.cpp)

set(DBSERVICE_TARGET test_tobasasql_dbsvc)
add_executable(${DBSERVICE_TARGET} dbservice.cpp)

set(POOL_TARGET test_tobasasql_pool)
add_executable(${POOL_TARGET} pool.cpp)

if(TOBASA_SQL_USE_MYSQL)
   set(MYSQL_TARGET test_tobasasql_mysql)
   add_executable(${MYSQL_TARGET} test_mysql.cpp)
endif()   

if(MSVC)
   set_property(TARGET ${PROJECT_NAME} PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
   set_property(TARGET ${CONNECTOR_TARGET} PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
   set_property(TARGET ${DBSERVICE_TARGET} PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
   set_property(TARGET ${POOL_TARGET} PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
   
   if(TOBASA_SQL_USE_MYSQL)
      set_property(TARGET ${MYSQL_TARGET} PROPERTY
         MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
   endif()   
endif()

target_link_libraries(${PROJECT_NAME}     PRIVATE tobasa tobasasql)
target_link_libraries(${CONNECTOR_TARGET} PRIVATE tobasa tobasasql)
target_link_libraries(${DBSERVICE_TARGET} PRIVATE tobasa tobasasql)
target_link_libraries(${POOL_TARGET}      PRIVATE tobasa tobasasql)

if(TOBASA_SQL_USE_MYSQL)
   target_link_libraries(${MYSQL_TARGET}     PRIVATE tobasa tobasasql)
endif()

if(CMAKE_CXX20_COMPILE_FEATURES)
   target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
   target_compile_features(${CONNECTOR_TARGET} PRIVATE cxx_std_20)
   target_compile_features(${DBSERVICE_TARGET} PRIVATE cxx_std_20)
   target_compile_features(${POOL_TARGET} PRIVATE cxx_std_20)
   if(TOBASA_SQL_USE_MYSQL)
      target_compile_features(${MYSQL_TARGET} PRIVATE cxx_std_20)
   endif()
else()
   target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
   target_compile_features(${CONNECTOR_TARGET} PRIVATE cxx_std_17)
   target_compile_features(${DBSERVICE_TARGET} PRIVATE cxx_std_17)
   target_compile_features(${POOL_TARGET} PRIVATE cxx_std_17)
   if(TOBASA_SQL_USE_MYSQL)
      target_compile_features(${MYSQL_TARGET} PRIVATE cxx_std_17)
   endif()
endif()


# libpg required shared libraries
# -------------------------------------------------------
if(TOBASA_SQL_USE_PGSQL AND MSVC AND NOT TOBASA_BUILD_WITH_VCPKG)
   # For Pgsql 11.7 x64
   if( ${TOBASA_LIBPQ_DIR} STREQUAL "D:/programs/PostgreSQL/11.7_x64" )
      message(STATUS "App use libpq from: ${TOBASA_LIBPQ_DIR} - ${PROJECT_NAME}")
      set(LIBPQ_BIN_DLLS libcrypto-1_1-x64.dll libiconv-2.dll libintl-9.dll libpq.dll libssl-1_1-x64.dll libwinpthread-1.dll)

      foreach(DLL IN LISTS LIBPQ_BIN_DLLS)
         add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${TOBASA_LIBPQ_DIR}/bin/${DLL} $<TARGET_FILE_DIR:${PROJECT_NAME}>/)
      endforeach()
   endif()

   # For Pgsql 10.19 x32
   if( ${TOBASA_LIBPQ_DIR} STREQUAL "D:/programs/PostgreSQL/10.19_x86" )
      message(STATUS "App use libpq from: ${TOBASA_LIBPQ_DIR} - ${PROJECT_NAME}")
      set(LIBPQ_BIN_DLLS libpq.dll libintl-8.dll libiconv-2.dll libcrypto-1_1.dll libssl-1_1.dll)

      foreach(DLL IN LISTS LIBPQ_BIN_DLLS)
         add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${TOBASA_LIBPQ_DIR}/bin/${DLL} $<TARGET_FILE_DIR:${PROJECT_NAME}>/)
      endforeach()
   endif()
endif()


# Copy libmariadb.dll
# -------------------------------------------------------
if(TOBASA_SQL_USE_MYSQL AND TOBASA_SQL_USE_MARIADB_DLL)
   if(MSVC AND NOT TOBASA_BUILD_WITH_VCPKG)
      add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy
         ${TOBASA_FRAMEWORK_BINARY_DIR}/bin/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>/libmariadb.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>/)
   endif()
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