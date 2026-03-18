#include <memory>
#include <vector>
#include <string>
#include "tobasalis/lis/record.h"

namespace tbs {
namespace lis {

Record::Record()
{
   _lisString  = "";
   _totalField = 0;
   _pParent    = nullptr;
   _pChildren  = nullptr;
   _pNext      = nullptr;
}

Record::Record(const std::string& lisString)
   : Record()
{
   _lisString = lisString;
}

} // namespace lis
} // namespace tbs
