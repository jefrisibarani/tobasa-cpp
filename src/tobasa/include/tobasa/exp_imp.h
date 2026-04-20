#pragma once

#if defined(_WIN32) || defined(_WIN64)
   #if defined(_MSC_VER)
      #define TBS_EXPORT __declspec(dllexport)
      #define TBS_IMPORT __declspec(dllimport)
   #elif defined(__GNUC__) 
      #define TBS_EXPORT __attribute__((dllexport))
      #define TBS_IMPORT __attribute__((dllimport))
   #else
      #define TBS_EXPORT
      #define TBS_IMPORT
   #endif
#else
   #if defined(__GNUC__) || defined(__clang__)
      #define TBS_EXPORT __attribute__ ((visibility("default")))
      #define TBS_IMPORT __attribute__ ((visibility("default")))
   #else
      #define TBS_EXPORT
      #define TBS_IMPORT
   #endif
#endif


#ifdef TBS_DLL
   #define TBS_API TBS_EXPORT
#else
   #define TBS_API TBS_IMPORT
#endif


#define TBS_PLUGIN extern "C" TBS_EXPORT