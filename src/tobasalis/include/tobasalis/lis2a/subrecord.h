#pragma once

#include "tobasalis/exp_imp.h"

#include "tobasalis/lis2a/record.h"

namespace tbs {
namespace lis2a {

/** \ingroup LIS
 * SubRecord
 */
class TBS_LIS_API SubRecord
   : public Record
{
public:
   SubRecord(const std::string& aLisString = "");
   ~SubRecord() = default;

protected:
   virtual void initFields() {};
};

} // namespace lis2a
} // namespace tbs 