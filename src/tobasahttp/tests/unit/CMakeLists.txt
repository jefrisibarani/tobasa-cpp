include(GoogleTest)

add_executable(tobasahttp_unit_tests
   methods_test.cpp
   http_parser_test.cpp
   http_multipart_parser_test.cpp
)
set_target_properties(tobasahttp_unit_tests PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
   LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
   ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
)
target_link_libraries(tobasahttp_unit_tests PRIVATE tobasahttp GTest::gtest_main)
gtest_discover_tests(tobasahttp_unit_tests)