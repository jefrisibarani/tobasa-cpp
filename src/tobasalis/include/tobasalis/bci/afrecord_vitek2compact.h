#pragma once

#include "tobasalis/exp_imp.h"

#include "tobasalis/bci/record.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * AfRecordVitek2Compact
 */
class TBS_LIS_API AfRecordVitek2Compact
   : public Record
{
protected:
   virtual void initFields();

public:
   AfRecordVitek2Compact(const std::string& bciString = "");
   ~AfRecordVitek2Compact() = default;
};

class TBS_LIS_API ApRecordVitek2Compact : public Record
{
protected:
   virtual void initFields();

public:
   ApRecordVitek2Compact(const std::string& bciString = "");
   ~ApRecordVitek2Compact() = default;
};

} // namespace bci
} // namespace tbs 