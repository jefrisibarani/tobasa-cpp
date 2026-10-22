#pragma once

#include "tobasalis/exp_imp.h"

#include "tobasalis/lis/message.h"
#include "tobasalis/bci/record.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * BciMessage
 */
class TBS_LIS_API BciMessage
   : public lis::Message
{
public:
   BciMessage();
   virtual ~BciMessage() = default;

   void addTest(const lis::RecordPtr& test);
   std::shared_ptr<Record> getTest();
   virtual std::string toString();
};


/** \ingroup LIS
 * VidasMessage
 */
class TBS_LIS_API VidasMessage
   : public lis::Message
{
public:
   VidasMessage();
   virtual ~VidasMessage();
   virtual std::string toString();
};


} // namespace bci
} // namespace tbs 