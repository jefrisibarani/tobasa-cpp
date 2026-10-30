#pragma once

#include <chrono>
#include <version>

#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L

   #ifndef TOBASA_USE_STD_DATE
      #define TOBASA_USE_STD_DATE
   #endif

#endif

#ifdef TOBASA_USE_STD_DATE

   namespace tbsdate = std::chrono;

#else

   #include <date/date.h>
   #include <date/tz.h>

   namespace tbsdate = date;

#endif