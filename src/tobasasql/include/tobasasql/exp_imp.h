#pragma once

#include <tobasa/exp_imp.h>

#if defined(TBS_SQL_STATIC)
   #define TBS_SQL_API
#elif defined(TBS_SQL_SHARED)
   #define TBS_SQL_API TBS_EXPORT
#else
   #define TBS_SQL_API TBS_IMPORT
#endif