#pragma once

namespace tbs {
namespace log {

/** 
 * \ingroup SQL
 * @brief Null Logger logs nothing.
 */
class NullLogger 
{
public:

   /// Log Trace message.
   template<class... Args>
   void trace(Args&&... args) {}

   /// Log Debug message.
   template<class... Args>
   void debug(Args&&... args) {}

   /// Log Information message.
   template<class... Args>
   void info(Args&&... args) {}

   /// Log Warning message.
   template<class... Args>
   void warn(Args&&... args) {}

   /// Log Error message.
   template<class... Args>
   void error(Args&&... args) {}
};

} // namespace log
} // namespace tbs