add_library(tobasa_test_support STATIC
   src/environment.cpp
   src/util.cpp
   include/tobasa_test_support/environment.h
   include/tobasa_test_support/util_u8.h
   include/tobasa_test_support/util.h
)

target_include_directories(tobasa_test_support
   PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(tobasa_test_support
   PRIVATE tobasa
)

if(MSVC)
   set_property(TARGET tobasa_test_support PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()