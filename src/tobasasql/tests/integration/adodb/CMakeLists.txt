include(GoogleTest)

# ----------------------------------------------------------------------
add_executable(tbs_adodb_integration_tests
   generic_api_test.cpp
   adodb_api_test.cpp
   command_reuse_test.cpp
)
set_target_properties(tbs_adodb_integration_tests PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
   LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
   ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
)
target_link_libraries(tbs_adodb_integration_tests
   PRIVATE tobasasql tobasa_test_support GTest::gtest_main
)
target_compile_definitions(tbs_adodb_integration_tests PRIVATE
   TOBASA_PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
target_compile_definitions(tbs_adodb_integration_tests PRIVATE
   TOBASA_PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
gtest_discover_tests(tbs_adodb_integration_tests)