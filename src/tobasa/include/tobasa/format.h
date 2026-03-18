#pragma once

#ifdef __GNUC__ 
   #ifndef TBS_GCC_VERSION
      #define TBS_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
   #endif
#else
   #ifndef TBS_GCC_VERSION
      #define TBS_GCC_VERSION 0
   #endif
#endif
//#if TBS_GCC_VERSION >= 1400

#include <chrono>

#if (defined(_MSC_VER) && _MSC_VER >= 1930) || \
    (defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L) && __cplusplus >= 202002L

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