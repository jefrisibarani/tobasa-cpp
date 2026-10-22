include(GoogleTest)

add_executable(tobasaweb_unit_tests
   result_test.cpp
   csrf_middleware_test.cpp
   session_store_test.cpp
)
set_target_properties(tobasaweb_unit_tests PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
   LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
   ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
)
target_link_libraries(tobasaweb_unit_tests
   PRIVATE tobasaweb tobasa tobasahttp tobasasql GTest::gtest_main
)
tobasa_stage_shared_runtime(tobasaweb_unit_tests tobasaweb tobasa tobasahttp tobasasql)
gtest_discover_tests(tobasaweb_unit_tests)