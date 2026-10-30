#pragma once

#include "tobasa/json.h"
#include "tobasa/logger_level.h"

namespace tbs {
namespace log {

NLOHMANN_JSON_SERIALIZE_ENUM(LogLevel, {
      {trace,    "trace"},
      {debug,    "debug"},
      {info,     "info"},
      {warn,     "warn"},
      {err,      "error"},
      {critical, "critical"},
      {off,      "off"},
      {n_levels, "n_level"},
   })


/// Configuration option classes
namespace conf {

/** \addtogroup WEB
 * @{
 */

/// Stdout log options
struct LogStdout
{
   LogLevel level;       ///< logging level
   std::string pattern;  ///< logging pattern
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LogStdout, level, pattern)


/// Log to file options
struct LogBasicFile
{
   LogLevel level;       ///< logging level
   std::string pattern;  ///< logging pattern
   std::string filePath; ///< Path to file
   bool truncate;        ///< Truncate file
   bool enable;          ///< Enable this file sink
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LogBasicFile, level, pattern, filePath, truncate, enable)


/// Logging options
struct Logging
{
   LogStdout stdoutColor;   ///< log stdout color
   LogBasicFile fileSink;   ///< log to file log info sub option
   LogBasicFile fileSinkD;  ///< log to file log debug sub option
   LogLevel multiSinkLevel; ///< multiple sink logging level
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Logging, stdoutColor, fileSink, fileSinkD, multiSinkLevel)

/** @}*/

} // namespace conf
} // namespace log
} // namespace tbs
