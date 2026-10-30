#pragma once

#include <version>

#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L

   #ifndef TOBASA_USE_STD_FORMAT
      #define TOBASA_USE_STD_FORMAT
   #endif

#endif

#ifdef TOBASA_USE_STD_FORMAT

   #include <format>
   namespace tbsfmt = std;

#else

   #include <fmt/core.h>
   #include <fmt/ostream.h>
   #include <fmt/chrono.h>
   namespace tbsfmt = fmt;

#endif