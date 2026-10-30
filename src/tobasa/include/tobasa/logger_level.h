#pragma once

namespace tbs {
namespace log {

enum LogLevel : int
{
   trace    = 0,
   debug    = 1,
   info     = 2,
   warn     = 3,
   err      = 4,
   critical = 5,
   off      = 6,
   n_levels
};


} // namespace log
} // namespace tbs   