#pragma once

#include <tobasa/exp_imp.h>

#if defined(TBS_HTTP_STATIC)
   #define TBS_HTTP_API
#elif defined(TBS_HTTP_SHARED)
   #define TBS_HTTP_API TBS_EXPORT
#else
   #define TBS_HTTP_API TBS_IMPORT
#endif