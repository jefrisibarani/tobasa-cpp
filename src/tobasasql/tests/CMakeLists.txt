if(NOT TOBASA_BUILD_TESTS)
   return()
endif()

add_subdirectory(unit)
add_subdirectory(integration)
add_subdirectory(stress)