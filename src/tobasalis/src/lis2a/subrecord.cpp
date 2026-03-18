#include "tobasalis/lis2a/subrecord.h"

namespace tbs {
namespace lis2a {

SubRecord::SubRecord(const std::string& lisString)
   : Record(lisString)
{
   setRecordType(RecordType::SubRecord);
   _isSubRecord = true;
}

} // namespace lis2a
} // namespace tbs