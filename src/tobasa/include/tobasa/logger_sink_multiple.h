#pragma once

#include "tobasa/exp_imp.h"
#include "tobasa/logger_sink.h"
#include "tobasa/settings_log.h"

namespace tbs {
/// Contains logging target/implementation class
namespace log {

/**
 * \ingroup WEB
 * \brief Implements actual logging using spdlog multi_sink target.
 * The MultipleLogSink class serves as an implementation for actual logging
 * utilizing the spdlog multi_sink target. Instances of this class can be used
 * as a parameter for tbs::Logger::setTarget().
 */

class TBS_API MultipleLogSink
   : public LogSink
{
private:
   struct LogImpl;
   std::unique_ptr<LogImpl> _logImpl;

public:

   /**
    * \brief Constructs a MultipleLogSink instance.
    * Initializes the spdlog multi_sink target by configuring a 'basic_file_sink_mt'
    * and a 'console_sink'. The 'basic_file_sink_mt' utilizes two target files:
    * one for debug logs and another for informational logs.
    * \param option Configuration settings for logging provided by 'conf::Logging'.
    */   
   MultipleLogSink(const conf::Logging& option);

   /// Destructor
   ~MultipleLogSink();

   /// Log Trace message
   void trace(const std::string& message);

   /// Log Debug message
   void debug(const std::string& message);

   /// Log Information message
   void info(const std::string& message);

   /// Log Warning message
   void warn(const std::string& message);

   /// Log Error message
   void error(const std::string& message);
};

} // namespace log
} // namespace tbs