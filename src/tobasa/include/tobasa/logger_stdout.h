#pragma once

#include <string>
#include "tobasa/format.h"

namespace tbs {
namespace log {

enum Level : uint8_t {
   None  = 0,
   Error = 1 << 0, // 00001
   Warn  = 1 << 1, // 00010
   Info  = 1 << 2, // 00100
   Debug = 1 << 3, // 01000
   Trace = 1 << 4, // 10000

   // Convenience masks: each includes everything below
   ErrorMask = Error,
   WarnMask  = Error | Warn,
   InfoMask  = Error | Warn | Info,
   DebugMask = Error | Warn | Info | Debug,
   TraceMask = Error | Warn | Info | Debug | Trace
};

class CoutLogSink;

/**
 * @brief Logging implementation to stdout.
 * All log methods trace(), debug(), info(), warn() and error()
 * use parameter like \p std::format does. \n
 * Example:
 * @code
 *      logger.trace("The number is {} and {}", 2 , 4);
 *      logger.info("Informational message");
 * @endcode
 */
struct StdoutLogger
{
public:

   StdoutLogger( const StdoutLogger & ) = delete;
   StdoutLogger & operator = ( const StdoutLogger & ) = delete;

   StdoutLogger();
   ~StdoutLogger();

   /// Log Trace message
   template<typename FormatString, typename... Args>
   void trace(const FormatString& fmt, const Args&... args)
   {
      if (_level & Trace)
         logT(tbsfmt::vformat(fmt, tbsfmt::make_format_args(args...)));
   }

   /// Log Debug message
   template<typename FormatString, typename... Args>
   void debug(const FormatString& fmt, const Args&... args)
   {
      if (_level & Debug)
         logD(tbsfmt::vformat(fmt, tbsfmt::make_format_args(args...)));
   }

   /// Log Information message
   template<typename FormatString, typename... Args>
   void info(const FormatString& fmt, const Args&... args)
   {
      if (_level & Info)
         logI(tbsfmt::vformat(fmt, tbsfmt::make_format_args(args...)));
   }

   /// Log Warning message
   template<typename FormatString, typename... Args>
   void warn(const FormatString& fmt, const Args&... args)
   {
      if (_level & Warn)
         logW(tbsfmt::vformat(fmt, tbsfmt::make_format_args(args...)));
   }

   /// Log Error message
   template<typename FormatString, typename... Args>
   void error(const FormatString& fmt, const Args&... args)
   {
      if (_level & Error)
         logE(tbsfmt::vformat(fmt, tbsfmt::make_format_args(args...)));
   }

   void setLevel(Level level) { _level = level; }

private:

   void logT(const std::string& msg);
   void logD(const std::string& msg);
   void logI(const std::string& msg);
   void logW(const std::string& msg);
   void logE(const std::string& msg);

   CoutLogSink* _sink;

   Level _level = Level::InfoMask;   
};

} // namespace log
} // namespace tbs