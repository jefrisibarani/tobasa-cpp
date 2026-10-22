#pragma once

#include "tobasalis/exp_imp.h"

#include "tobasalis/lis/message.h"

namespace tbs {
namespace dirui {

/** \ingroup LIS
 * Dirui Message
 */
class TBS_LIS_API Message
   : public lis::Message
{
public:
   Message();
   virtual ~Message() = default;
   std::string toString();
};

} // namespace dirui
} // namespace tbs