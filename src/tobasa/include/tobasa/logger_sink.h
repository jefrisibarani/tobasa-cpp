#pragma once

#include <string>
#include <iostream>
#include <mutex>

namespace tbs {
namespace log {

/** \ingroup TBS
 * @{
 */

/** 
 * @brief Base class for logging sink class.
 * Default implementation do nothing.
 */
class LogSink
{
public:
   LogSink() = default;
   virtual ~LogSink() = default;

   /// Log Trace message
   virtual void trace(const std::string& message) {}

   /// Log Debug message
   virtual void debug(const std::string& message) {}

   /// Log Information message
   virtual void info(const std::string& message) {}

   /// Log Warning message
   virtual void warn(const std::string& message) {}

   /// Log Error message
   virtual void error(const std::string& message) {}
};


/// Implement logging to stdout.
class CoutLogSink : public LogSink
{
public:
   CoutLogSink(const CoutLogSink&) = delete;
   CoutLogSink& operator = (const CoutLogSink&) = delete;

   CoutLogSink();
   ~CoutLogSink();

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

private:
   void doLog(const std::string& tag, const std::string& msg);
   std::mutex _lock;
   std::ostream* _out;
};

/** @}*/

} // namespace log
} // namespace tbs