include(GoogleTest)

# ----------------------------------------------------------------------
add_executable(tbs_sqlite_integration_tests
   generic_api_test.cpp
   sqlite_api_test.cpp
   command_reuse_test.cpp
)
set_target_properties(tbs_sqlite_integration_tests PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
   LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
   ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
)
target_link_libraries(tbs_sqlite_integration_tests
   PRIVATE tobasasql tobasa_test_support GTest::gtest_main
)
target_compile_definitions(tbs_sqlite_integration_tests PRIVATE
   TOBASA_PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
gtest_discover_tests(tbs_sqlite_integration_tests)


# # ----------------------------------------------------------------------
# add_executable(tobasasql_sqlite_command_integration_tests
#    sqlite_command_reuse_test.cpp
# )
# set_target_properties(tobasasql_sqlite_command_integration_tests PROPERTIES
#    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
#    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
#    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
# )
# target_link_libraries(tobasasql_sqlite_command_integration_tests
#    PRIVATE tobasasql GTest::gtest_main
# )
# target_compile_definitions(tobasasql_sqlite_command_integration_tests PRIVATE
#    TOBASA_PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
# )
# gtest_discover_tests(tobasasql_sqlite_command_integration_tests)

