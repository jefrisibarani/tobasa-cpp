file(GLOB MY_SOURCES_LIST CONFIGURE_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/src/tobasaweb/*.cpp
   )

file(GLOB MY_HEADERS_LIST CONFIGURE_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasaweb/*.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasaweb/dto/*.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasaweb/entity/*.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasaweb/db_migrations/*.h
   )