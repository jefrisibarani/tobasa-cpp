include(GoogleTest)

add_executable(tobasalis_unit_tests requestinfo_test.cpp)
set_target_properties(tobasalis_unit_tests PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
   LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
   ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
)
target_link_libraries(tobasalis_unit_tests PRIVATE tobasalis GTest::gtest_main)
gtest_discover_tests(tobasalis_unit_tests)