#pragma once

#include <tobasa/exp_imp.h>

#if defined(TBS_WEB_STATIC)
   #define TBS_WEB_API
#elif defined(TBS_WEB_SHARED)
   #define TBS_WEB_API TBS_EXPORT
#else
   #define TBS_WEB_API TBS_IMPORT
#endif