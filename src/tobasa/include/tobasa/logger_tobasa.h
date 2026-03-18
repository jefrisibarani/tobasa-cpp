#pragma once

#include "tobasa/logger.h"

namespace tbs {
namespace log {

/**
 * @brief Logging implementation to tbs::Logger
 * Provides logging functionality with methods for tracing,
 * debugging, information, warning, and error messages. These methods use parameters
 * in a similar way to \p std::format. \n
 * 
 * This logger uses tbs::Logger class, so we must first set tbs::Logger actual target 
 * Example usage:
 * @code
 *    // we must set actual log target
 *    tbs::Logger::setTarget(new tbs::log::CoutLogSink());
 * 
 *    logger.trace("The numbers are {} and {}", 2, 4);
 *    logger.info("Informational message");
 * @endcode
 */
struct TobasaLogger
{
public:

   /// Log Trace message
   template<class... Args>
   void trace(Args&&... args)
   {
      tbs::Logger::logT(std::forward<Args>(args)...);
   }

   /// Log Debug message
   template<class... Args>
   void debug(Args&&... args)
   {
      tbs::Logger::logD(std::forward<Args>(args)...);
   }

   /// Log Information message
   template<class... Args>
   void info(Args&&... args)
   {
      tbs::Logger::logI(std::forward<Args>(args)...);
   }

   /// Log Warning message
   template<class... Args>
   void warn(Args&&... args)
   {
      tbs::Logger::logW(std::forward<Args>(args)...);
   }

   /// Log Error message
   template<class... Args>
   void error(Args&&... args)
   {
      tbs::Logger::logE(std::forward<Args>(args)...);
   }
};

} // namespace log
} // namespace tbs