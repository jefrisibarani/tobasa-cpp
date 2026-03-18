#pragma once

#include <string>
#include "tobasa/format.h"
#include "tobasa/path.h"
#include "tobasa/logger_sink.h"

namespace tbs {

/** 
 * @ingroup TBS
 * @brief Logging driver class.
 *
 * This class uses a LogSink instance which implement actual logging.
 * Use setTarget() to set LogSink derived class instance,
 * and enabling logging with enableLogging().
 *
 * All logging methods, logT(), logD(), logI(), logW() and logE()
 * use parameter like tbsfmt::format does.
 * Example:
 * @code
 *     // we must set actual log target
 *     tbs::Logger::setTarget(new tbs::log::CoutLogSink());
 * 
 *     Logger::logD("The number is {} and {}", 2 , 4);
 *     Logger::logI("Informational message");
 * @endcode
 */
class Logger
{
public:
   Logger(const Logger&) = delete;
   const Logger& operator=(const Logger&) = delete;
   Logger(Logger&&) = delete;
   const Logger& operator=(Logger&&) = delete;

   /// Enable logging
   static void enableLogging();

   /// Disable logging
   static void disableLogging();

   /// Get log file path
   static std::string logFilePath();

   /// Set log file path
   static void logFilePath(const std::string& logFilePath);

   static void destroyInternalLogSink();

   /**
    * Set log sink target
    * \c newTarget will replace current target.
    * This class owns logging target in \c std:unique_ptr
    * You must instantiate log target inside this function parameter
    * \code
    *   Logger::setTarget(new CoutLogSink());
    * \endcode
    */
   static void setTarget(log::LogSink* newTarget);

   /// Log Trace
   template<typename FormatString, typename... Args>
   static void logT(const FormatString& fmt, const Args&... args)
   {
      if (_enableLog && _pTarget)
         _pTarget->trace(tbsfmt::vformat(fmt, tbsfmt::make_format_args(args...)));
   }
   /// Log Trace
   static void logT(const std::string& message);

   /// Log Debug
   template<typename FormatString, typename... Args>
   static void logD(const FormatString& fmt, const Args&... args)
   {
      if (_enableLog && _pTarget)
         _pTarget->debug(tbsfmt::vformat(fmt, tbsfmt::make_format_args(args...)));
   }
   /// Log Debug
   static void logD(const std::string& message);

   /// Log Information
   template<typename FormatString, typename... Args>
   static void logI(const FormatString& fmt, const Args&... args)
   {
      if (_enableLog && _pTarget)
         _pTarget->info(tbsfmt::vformat(fmt, tbsfmt::make_format_args(args...)));
   }
   /// Log Information
   static void logI(const std::string& message);

   /// Log Warning
   template<typename FormatString, typename... Args>
   static void logW(const FormatString& fmt, const Args&... args)
   {
      if (_enableLog && _pTarget)
         _pTarget->warn(tbsfmt::vformat(fmt, tbsfmt::make_format_args(args...)));
   }
   /// Log Warning
   static void logW(const std::string& message);

   /// Log Error
   template<typename FormatString, typename... Args>
   static void logE(const FormatString& fmt, const Args&... args)
   {
      if (_enableLog && _pTarget)
         _pTarget->error(tbsfmt::vformat(fmt, tbsfmt::make_format_args(args...)));
   }
   /// Log Error
   static void logE(const std::string& message);

protected:
   // c++ 17 allow inline static
   
   /// Logger _enableLog default to true
   inline static bool _enableLog = true;

   /// Logger sink
   static std::unique_ptr<log::LogSink> _pTarget;

   inline static std::string _logFilePath;
};

} // namespace tbs