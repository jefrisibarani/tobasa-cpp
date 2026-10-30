macro(tobasa_configure_date_and_timezone_lib)

   if((TOBASA_FIND_DEPS OR TOBASA_BUILD_WITH_VCPKG) AND NOT TOBASA_BUILD_IN_MEMORY_TZDB)
      find_package(date CONFIG REQUIRED)
      set(TOBASA_USE_BUNDLED_TZ_LIB FALSE CACHE INTERNAL "" )
   else()
      set(TOBASA_USE_BUNDLED_TZ_LIB TRUE CACHE INTERNAL "" )
      message(STATUS "tobasa Using bundled timezone library")

      if(TOBASA_BUILD_IN_MEMORY_TZDB)
         add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/resources/tzdata_assets.cpp"
            COMMAND generate_tzdata_assets_file ${TOBASA_FRAMEWORK_ROOTDIR}/res/tzdata tzdata ${CMAKE_CURRENT_BINARY_DIR}/resources/tzdata_assets.cpp getTzdataAsset tzdata
            DEPENDS generate_tzdata_assets_file
         )
      endif()

      add_library(timezone STATIC)
      target_sources( timezone PRIVATE ${TOBASA_EXTERNAL_DEPS_DIR}/date/src/tz.cpp )
      
      if(TOBASA_BUILD_IN_MEMORY_TZDB)
         target_sources( timezone PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/resources/tzdata_assets.cpp )
      endif()   

      target_include_directories(timezone PRIVATE ${TOBASA_EXTERNAL_DEPS_DIR}/date/include )

      if(TOBASA_BUILD_IN_MEMORY_TZDB)
         target_include_directories(timezone 
            PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
            PRIVATE ${TOBASA_FRAMEWORK_ROOTDIR}/src/tobasa/include)
      endif()
      
      target_compile_definitions(timezone PRIVATE HAS_REMOTE_API=0)

      if (WIN32 )
         if(TOBASA_BUILD_IN_MEMORY_TZDB)
            target_compile_definitions(timezone PRIVATE USE_IN_MEMORY_TZDB=1)
         endif()
      else() 
         if(TOBASA_BUILD_IN_MEMORY_TZDB)
            target_compile_definitions(timezone PRIVATE USE_IN_MEMORY_TZDB=1)
         else()
            target_compile_definitions(timezone PRIVATE USE_OS_TZDB=1)
         endif()
      endif()

      if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
         target_compile_options(timezone PRIVATE -fpic -c -Wall)
      endif()

      if(CMAKE_CXX20_COMPILE_FEATURES)
         target_compile_features(timezone PUBLIC cxx_std_20)
      else()
         target_compile_features(timezone PUBLIC cxx_std_17)
      endif()

   endif()

endmacro()