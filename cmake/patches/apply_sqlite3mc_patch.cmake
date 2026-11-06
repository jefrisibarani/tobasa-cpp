set(TARGET_FILE "${TOBASA_SQLITE3MC_SOURCE_DIR}/CMakeLists.txt")

if(NOT EXISTS "${TARGET_FILE}")
   message(FATAL_ERROR
      "SQLite3MC CMakeLists.txt not found: ${TARGET_FILE}"
   )
endif()

file(READ "${TARGET_FILE}" CONTENTS)

set(OLD_TEXT "if(ZLIB_FOUND)")
set(NEW_TEXT "if(ZLIB_FOUND AND ((SQLITE_ENABLE_COMPRESS OR SQLITE_ENABLE_SQLAR OR SQLITE_ENABLE_ZIPFILE)))")

# Already patched?
if(CONTENTS MATCHES
   "if\\(ZLIB_FOUND AND \\(\\(SQLITE_ENABLE_COMPRESS OR SQLITE_ENABLE_SQLAR OR SQLITE_ENABLE_ZIPFILE\\)\\)\\)"
)
   message(STATUS "tobasa SQLite3MC patch already applied")
else()
   # Expect exactly two occurrences in the original file.
   string(REGEX MATCHALL "if\\(ZLIB_FOUND\\)" MATCHES "${CONTENTS}")
   list(LENGTH MATCHES MATCH_COUNT)

   if(NOT MATCH_COUNT EQUAL 2)
      message(FATAL_ERROR
         "Expected 2 occurrences of 'if(ZLIB_FOUND)' in SQLite3MC "
         "CMakeLists.txt, but found ${MATCH_COUNT}. "
         "The source may have changed.")
   endif()

   string(REPLACE "${OLD_TEXT}" "${NEW_TEXT}" CONTENTS "${CONTENTS}")

   file(WRITE "${TARGET_FILE}" "${CONTENTS}")

   message(STATUS "tobasa Applied SQLite3MC ZLIB condition patch")
endif()