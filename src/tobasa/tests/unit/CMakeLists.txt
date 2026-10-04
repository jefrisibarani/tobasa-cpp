include(GoogleTest)

add_executable(tobasa_unit_tests
   uuid_test.cpp
   datetime_test.cpp
   path_test.cpp
   file_reader_test.cpp
   string_reader_test.cpp
   bytes_reader_test.cpp
   util_test.cpp
   util_date_test.cpp
   util_string_test.cpp
   util_time_test.cpp
   util_utf_test.cpp
)
set_target_properties(tobasa_unit_tests PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
   LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
   ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
)
target_link_libraries(tobasa_unit_tests PRIVATE tobasa GTest::gtest_main)

if(NOT TOBASA_USE_STD_CALENDAR_TZONE AND NOT TOBASA_BUILD_IN_MEMORY_TZDB)
   add_custom_command(TARGET tobasa_unit_tests POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
         ${TOBASA_FRAMEWORK_ROOTDIR}/res/tzdata/ $<TARGET_FILE_DIR:tobasa_unit_tests>/tzdata/)
endif()

gtest_discover_tests(tobasa_unit_tests)