set(TARGET_FILE "${TOBASA_MINIZIP_SOURCE_DIR}/CMakeLists.txt")

if(NOT EXISTS "${TARGET_FILE}")
   message(FATAL_ERROR "Minizip CMakeLists.txt not found: ${TARGET_FILE}")
endif()

file(READ "${TARGET_FILE}" CONTENTS)

set(OLD_SRCS [=[set(LIBMINIZIP_SRCS ioapi.c mztools.c unzip.c zip.c)]=])
set(NEW_SRCS [=[set(LIBMINIZIP_SRCS ioapi.c mztools.c unzip.c zip.c $<$<BOOL:${WIN32}>:iowin32.c> )]=])

set(OLD_HDRS [=[set(LIBMINIZIP_HDRS crypt.h ints.h ioapi.h mztools.h unzip.h zip.h)]=])
set(NEW_HDRS [=[set(LIBMINIZIP_HDRS crypt.h ints.h ioapi.h mztools.h unzip.h zip.h $<$<BOOL:${WIN32}>:iowin32.h>)]=])

string(FIND "${CONTENTS}" "${NEW_SRCS}" SRCS_PATCH)
string(FIND "${CONTENTS}" "${NEW_HDRS}" HDRS_PATCH)

if(NOT SRCS_PATCH EQUAL -1 AND NOT HDRS_PATCH EQUAL -1)
   message(STATUS "tobasa Minizip CMake patch already applied")
   return()
endif()

if(SRCS_PATCH EQUAL -1)
   string(FIND "${CONTENTS}" "${OLD_SRCS}" SRCS_SOURCE)
   if(SRCS_SOURCE EQUAL -1)
      message(FATAL_ERROR "Expected Minizip source declaration was not found in ${TARGET_FILE}")
   endif()

   string(REPLACE "${OLD_SRCS}" "${NEW_SRCS}" CONTENTS "${CONTENTS}")
endif()

if(HDRS_PATCH EQUAL -1)
   string(FIND "${CONTENTS}" "${OLD_HDRS}" HDRS_SOURCE)
   if(HDRS_SOURCE EQUAL -1)
      message(FATAL_ERROR "Expected Minizip header declaration was not found in ${TARGET_FILE}" )
   endif()

   string(REPLACE "${OLD_HDRS}" "${NEW_HDRS}" CONTENTS "${CONTENTS}")
endif()

file(WRITE "${TARGET_FILE}" "${CONTENTS}")
message(STATUS "tobasa Applied Minizip CMake patch")