include(GoogleTest)

add_executable(tobasasql_unit_tests
   mysql_command_basic_test.cpp
   sql_parameter_rewriter_test.cpp
   mysql_parameter_test.cpp
   sqlite_connection_test.cpp
)

set_target_properties(tobasasql_unit_tests PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
   LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
   ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
)

target_link_libraries(tobasasql_unit_tests PRIVATE tobasasql GTest::gtest_main)
gtest_discover_tests(tobasasql_unit_tests)