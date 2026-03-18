file(GLOB MY_SOURCES_LIST CONFIGURE_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/src/tobasahttp/*.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/tobasahttp/client/*.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/tobasahttp/server/*.cpp
   )

file(GLOB MY_HEADERS_LIST CONFIGURE_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasahttp/*.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasahttp/client/*.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasahttp/server/*.h
   )
