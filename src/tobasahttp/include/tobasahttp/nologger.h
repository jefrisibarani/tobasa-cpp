#pragma once

#include "tobasahttp/exp_imp.h"

namespace tbs {
namespace http {

/** 
 * \ingroup HTTP
 * Do nothing logger.
 */
struct TBS_HTTP_API NoLogger
{
public:
   template<class... Args>
   void trace(Args&&... args) {}

   template<class... Args>
   void debug(Args&&... args)  {}

   template<class... Args>
   void info(Args&&... args) {}

   template<class... Args>
   void warn(Args&&... args) {}

   template<class... Args>
   void error(Args&&... args) {}
};


} // namespace http
} // namespace tbs