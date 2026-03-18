#pragma once

#include <tobasa/json.h>
#include <spdlog/spdlog.h>

namespace spdlog {
namespace level {

NLOHMANN_JSON_SERIALIZE_ENUM(level_enum, {
      {trace,    "trace"},
      {debug,    "debug"},
      {info,     "info"},
      {warn,     "warn"},
      {err,      "error"},
      {critical, "critical"},
      {off,      "off"},
      {n_levels, "n_level"},
   })
} // namespace level
} // namespace spdlog


namespace tbs {
namespace log {
/// Configuration option classes
namespace conf {


/// Stdout log options
struct LogStdout
{
   spdlog::level::level_enum level;          ///< logging level
   std::string pattern;                      ///< logging pattern
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LogStdout, level, pattern)


/// Log to file options
struct LogBasicFile
{
   spdlog::level::level_enum level;          ///< logging level
   std::string pattern;                      ///< logging pattern
   std::string filePath;                     ///< Path to file
   bool truncate;                            ///< Truncate file
   bool enable;                              ///< Enable this file sink
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LogBasicFile, level, pattern, filePath, truncate, enable)


/// Logging options
struct Logging
{
   LogStdout stdoutColor;                    ///< log stdout color
   LogBasicFile fileSink;                    ///< log to file log info sub option
   LogBasicFile fileSinkD;                   ///< log to file log debug sub option
   spdlog::level::level_enum multiSinkLevel; ///< multi_sink logging level
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Logging, stdoutColor, fileSink, fileSinkD, multiSinkLevel)

} // namespace conf
} // namespace log
} // namespace tbs
