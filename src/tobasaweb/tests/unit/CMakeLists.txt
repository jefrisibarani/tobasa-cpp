include(GoogleTest)

add_executable(tobasaweb_unit_tests result_test.cpp)
set_target_properties(tobasaweb_unit_tests PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
   LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
   ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
)
target_link_libraries(tobasaweb_unit_tests
   PRIVATE tobasaweb tobasa tobasahttp GTest::gtest_main
)
gtest_discover_tests(tobasaweb_unit_tests)