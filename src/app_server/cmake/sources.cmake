# -------------------------------------------------------
if(TOBASA_BUILD_LIS_ENGINE)
   file(GLOB_RECURSE LIS_TEMPLATE_FILES "${PROJECT_SOURCE_DIR}/views_lis/lis/*.tpl")
   if(TOBASA_BUILD_IN_MEMORY_RESOURCES)
      add_custom_command(
         OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/resources/template_lis_resources.cpp"
         COMMAND generate_resources_file ${PROJECT_SOURCE_DIR}/views_lis views ${CMAKE_CURRENT_BINARY_DIR}/resources/template_lis_resources.cpp getTemplateLisResources appview_lis
         DEPENDS generate_resources_file ${LIS_TEMPLATE_FILES} )
   endif()
endif()

# -------------------------------------------------------
# Core Tobasa webapp resources
if(TOBASA_BUILD_IN_MEMORY_RESOURCES)

   file(GLOB_RECURSE APPSERVER_TEMPLATE_FILES "${PROJECT_SOURCE_DIR}/views/*.tpl")
   # HTML templates
   add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/resources/template_resources.cpp"
      COMMAND generate_resources_file ${PROJECT_SOURCE_DIR}/views views ${CMAKE_CURRENT_BINARY_DIR}/resources/template_resources.cpp getTemplateResources appview
      DEPENDS generate_resources_file ${APPSERVER_TEMPLATE_FILES} )

   # Webroot static files
   add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/resources/wwwroot_resources.cpp"
      COMMAND generate_resources_file ${PROJECT_SOURCE_DIR}/wwwroot wwwroot ${CMAKE_CURRENT_BINARY_DIR}/resources/wwwroot_resources.cpp getWwwrootResources wwwroot
      DEPENDS generate_resources_file )

endif()

# Embedded Default configuration files
file(GLOB_RECURSE APPSERVER_CONFIG_FILES "${PROJECT_SOURCE_DIR}/configuration_embed/*.json")
add_custom_command(
   OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/resources/config_resources.cpp"
   COMMAND generate_resources_file ${PROJECT_SOURCE_DIR}/configuration_embed config ${CMAKE_CURRENT_BINARY_DIR}/resources/config_resources.cpp getConfigResources config
   DEPENDS generate_resources_file ${APPSERVER_CONFIG_FILES} )
# message(STATUS "Found Application configuration files: ${APPSERVER_CONFIG_FILES}")

# Embedded Default TLS Certificates
add_custom_command(
   OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/resources/tls_asset_resources.cpp"
   COMMAND generate_resources_file ${PROJECT_SOURCE_DIR}/tls_asset tls_asset ${CMAKE_CURRENT_BINARY_DIR}/resources/tls_asset_resources.cpp getTlsAssetResources tls_asset
   DEPENDS generate_resources_file )

# Generate build_info.cpp
add_custom_command(
   OUTPUT "${PROJECT_SOURCE_DIR}/src/build_info.cpp"
   COMMAND generate_build_info ${PROJECT_SOURCE_DIR}/src/build_info.cpp
   DEPENDS generate_build_info ) 
   #COMMENT "Generating build_info.cpp")

# -------------------------------------------------------
file(GLOB APP_SOURCES_LIST 
   ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/middleware/*.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/antri/*.cpp )

file(GLOB APP_HEADERS_LIST CONFIGURE_DEPENDS
   ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
   ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.h
   ${CMAKE_CURRENT_SOURCE_DIR}/src/db_migrations/*.h
   ${CMAKE_CURRENT_SOURCE_DIR}/src/middleware/*.h
   ${CMAKE_CURRENT_SOURCE_DIR}/src/antri/*.h
   ${CMAKE_CURRENT_SOURCE_DIR}/src/antri/entity/*.h 
   ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/config_resources.h 
   ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/tls_asset_resources.h )

if(TOBASA_BUILD_IN_MEMORY_RESOURCES)
   file(GLOB APP_HEADERS_RESOURCES_LIST CONFIGURE_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/wwwroot_resources.h
      ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/template_resources.h )
endif()

# -------------------------------------------------------
if(TOBASA_BUILD_LIS_ENGINE)
   file(GLOB APP_LIS_SOURCES_LIST 
      ${CMAKE_CURRENT_SOURCE_DIR}/src/lis/*.cpp )

   file(GLOB APP_LIS_HEADERS_LIST CONFIGURE_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/src/lis/*.h 
      ${CMAKE_CURRENT_SOURCE_DIR}/src/lis/db_migrations/*.h )
   
   if(TOBASA_BUILD_IN_MEMORY_RESOURCES)
      file(GLOB APP_LIS_RESOURCES_HEADERS_LIST CONFIGURE_DEPENDS
         ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/template_lis_resources.h )
   endif()
endif()

# -------------------------------------------------------
if(TOBASA_BUILD_TESTS)
   file(GLOB APP_TEST_SOURCES_LIST 
      ${CMAKE_CURRENT_SOURCE_DIR}/src/test/*.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/test_ws/*.cpp )

   file(GLOB APP_TEST_HEADERS_LIST CONFIGURE_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/src/test/*.h
      ${CMAKE_CURRENT_SOURCE_DIR}/src/test_ws/*.h )

   if(TOBASA_BUILD_TESTS_WSCHAT_USE_PROBUF)
      file(GLOB APP_TEST_WSCHAT_PROBUF_HEADERS_LIST CONFIGURE_DEPENDS
         ${CMAKE_CURRENT_SOURCE_DIR}/src/test_ws/proto/*.h )

      file(GLOB APP_TEST_WSCHAT_PROBUF_SOURCES_LIST
         ${CMAKE_CURRENT_SOURCE_DIR}/src/test_ws/proto/*.cc )
   endif()
endif()
# -------------------------------------------------------

