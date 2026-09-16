file(GLOB MY_SOURCES_LIST CONFIGURE_DEPENDS
   ${CMAKE_CURRENT_SOURCE_DIR}/src/main_with_worker_threads.cpp
   #${CMAKE_CURRENT_SOURCE_DIR}/src/main_without_worker_threads.cpp
)

file(GLOB MY_HEADERS_LIST CONFIGURE_DEPENDS
   ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)