# Generate build_info.cpp
add_custom_command(
   OUTPUT "${PROJECT_SOURCE_DIR}/src/build_info.cpp"
   COMMAND generate_build_info ${PROJECT_SOURCE_DIR}/src/build_info.cpp
   DEPENDS generate_build_info ) 
   #COMMENT "Generating build_info.cpp")

file(GLOB MY_SOURCES_LIST CONFIGURE_DEPENDS
   ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

file(GLOB MY_HEADERS_LIST CONFIGURE_DEPENDS
   ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)