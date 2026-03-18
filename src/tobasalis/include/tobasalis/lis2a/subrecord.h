#pragma once

#include "tobasalis/lis2a/record.h"

namespace tbs {
namespace lis2a {

/** \ingroup LIS
 * SubRecord
 */
class SubRecord
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