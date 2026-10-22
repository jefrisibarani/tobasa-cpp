include(GoogleTest)

# ----------------------------------------------------------------------
add_executable(tbs_sqlite_integration_tests
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
   TOBASA_DB_TEST_SQLITE
)
gtest_discover_tests(tbs_sqlite_integration_tests)


# ----------------------------------------------------------------------
add_executable(tbs_sqlite_tpl_api_integration_tests
   template_api_test.cpp
)
set_target_properties(tbs_sqlite_tpl_api_integration_tests PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
   LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
   ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
)
target_link_libraries(tbs_sqlite_tpl_api_integration_tests
   PRIVATE tobasasql tobasa_test_support GTest::gtest_main
)
target_compile_definitions(tbs_sqlite_tpl_api_integration_tests PRIVATE
   TOBASA_PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
   TOBASA_DB_TEST_SQLITE
)
gtest_discover_tests(tbs_sqlite_tpl_api_integration_tests)


# ----------------------------------------------------------------------
add_executable(tbs_sqlite_db_api_integration_tests
   ../db_api_test.cpp
)
set_target_properties(tbs_sqlite_db_api_integration_tests PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
   LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
   ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
)
target_link_libraries(tbs_sqlite_db_api_integration_tests
   PRIVATE tobasasql tobasa_test_support GTest::gtest_main
)
target_compile_definitions(tbs_sqlite_db_api_integration_tests PRIVATE
   TOBASA_PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
   TOBASA_DB_TEST_SQLITE
)
gtest_discover_tests(tbs_sqlite_db_api_integration_tests)

foreach(test_target IN ITEMS
   tbs_sqlite_integration_tests
   tbs_sqlite_tpl_api_integration_tests
   tbs_sqlite_db_api_integration_tests)
   tobasa_stage_shared_runtime(${test_target} tobasasql tobasa)
endforeach()

