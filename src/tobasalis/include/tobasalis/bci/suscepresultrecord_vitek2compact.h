#pragma once

#include "tobasalis/bci/record.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * SusceptibilityResultRecordVitek2Compact
 */
class SusceptibilityResultRecordVitek2Compact
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