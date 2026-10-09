include(GoogleTest)

add_executable(tobasahttp_unit_tests
   http_client_test.cpp
   methods_test.cpp
   http_parser_test.cpp
   http_multipart_parser_test.cpp
   http_range_response_test.cpp
)
set_target_properties(tobasahttp_unit_tests PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
   LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
   ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
)
target_link_libraries(tobasahttp_unit_tests PRIVATE tobasahttp tobasa tobasa_test_support GTest::gtest_main)
target_compile_definitions(tobasahttp_unit_tests PRIVATE
   TOBASA_PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
gtest_discover_tests(tobasahttp_unit_tests)