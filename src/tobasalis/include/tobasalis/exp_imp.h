#pragma once

#include <tobasa/exp_imp.h>

#if defined(TBS_LIS_STATIC)
   #define TBS_LIS_API
#elif defined(TBS_LIS_SHARED)
   #define TBS_LIS_API TBS_EXPORT
#else
   #define TBS_LIS_API TBS_IMPORT
#endif