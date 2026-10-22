#pragma once

#include "tobasalis/exp_imp.h"

#include "tobasalis/bci/record.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * SusceptibilityResultRecordVitek2Compact
 */
class TBS_LIS_API SusceptibilityResultRecordVitek2Compact
   : public Record
{
protected:
   virtual void initFields();

public:
   SusceptibilityResultRecordVitek2Compact(const std::string& bciString = "");
   ~SusceptibilityResultRecordVitek2Compact() = default;
};

} // namespace bci
} // namespace tbs 