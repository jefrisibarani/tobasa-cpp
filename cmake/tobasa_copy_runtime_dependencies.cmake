if(NOT DEFINED TOBASA_RUNTIME_EXECUTABLE OR NOT DEFINED TOBASA_RUNTIME_DESTINATION)
   message(FATAL_ERROR "Runtime executable and destination must be specified")
endif()

string(REPLACE "|" ";" runtime_search_dirs "${TOBASA_RUNTIME_SEARCH_DIRS}")

file(GET_RUNTIME_DEPENDENCIES
   EXECUTABLES "${TOBASA_RUNTIME_EXECUTABLE}"
   DIRECTORIES ${runtime_search_dirs}
   RESOLVED_DEPENDENCIES_VAR resolved_dependencies
   UNRESOLVED_DEPENDENCIES_VAR unresolved_dependencies
   CONFLICTING_DEPENDENCIES_PREFIX conflicting_dependencies
   PRE_EXCLUDE_REGEXES
      "^api-ms-win-.*"
      "^ext-ms-.*"
   POST_EXCLUDE_REGEXES
      ".*[Ww]indows[/\\][Ss]ystem32[/\\].*"
      ".*[Ww]indows[/\\][Ss]ysWOW64[/\\].*")

foreach(runtime_dependency IN LISTS resolved_dependencies)
   execute_process(
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
         "${runtime_dependency}"
         "${TOBASA_RUNTIME_DESTINATION}"
      RESULT_VARIABLE copy_result)
   if(NOT copy_result EQUAL 0)
      message(FATAL_ERROR "Failed to stage runtime dependency: ${runtime_dependency}")
   endif()
endforeach()

if(unresolved_dependencies)
   message(WARNING
      "Unresolved runtime dependencies for ${TOBASA_RUNTIME_EXECUTABLE}: ${unresolved_dependencies}")
endif()

if(conflicting_dependencies_FILENAMES)
   message(WARNING
      "Conflicting runtime dependency names: ${conflicting_dependencies_FILENAMES}")
endif()