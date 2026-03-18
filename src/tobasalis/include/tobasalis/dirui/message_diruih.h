#pragma once

#include "tobasalis/lis/message.h"

namespace tbs {
namespace dirui {

/** \ingroup LIS
 * Dirui Message
 */
class Message
   : public lis::Message
{
public:
   Message();
   virtual ~Message() = default;
   std::string toString();
};

} // namespace dirui
} // namespace tbs