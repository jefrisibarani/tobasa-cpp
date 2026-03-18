file(GLOB MY_SOURCES_LIST CONFIGURE_DEPENDS
   ${CMAKE_CURRENT_SOURCE_DIR}/src/hl7/*.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/bci/*.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/dirui/*.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/combostik/*.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/lis/*.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/lis1a/*.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/lis2a/*.cpp
)

file(GLOB MY_HEADERS_LIST CONFIGURE_DEPENDS
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasalis/hl7/*.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasalis/bci/*.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasalis/dirui/*.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasalis/combostik/*.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasalis/lis/*.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasalis/lis1a/*.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasalis/lis2a/*.h
)