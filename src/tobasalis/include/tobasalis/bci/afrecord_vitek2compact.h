#pragma once

#include "tobasalis/bci/record.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * AfRecordVitek2Compact
 */
class AfRecordVitek2Compact
   : public Record
{
protected:
   virtual void initFields();

public:
   AfRecordVitek2Compact(const std::string& bciString = "");
   ~AfRecordVitek2Compact() = default;
};

class ApRecordVitek2Compact : public Record
{
protected:
   virtual void initFields();

public:
   ApRecordVitek2Compact(const std::string& bciString = "");
   ~ApRecordVitek2Compact() = default;
};

} // namespace bci
} // namespace tbs 