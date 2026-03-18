#include <string>
#include "tobasalis/bci/generalrecord_vitek2compact.h"

namespace tbs {
namespace bci {

GeneralRecordVitek2Compact::GeneralRecordVitek2Compact(const std::string& bciString)
   : Record(bciString)
{
   setRecordType(RecordType::General);
   initFields();
   _totalField = (int)_bciFields.size();
   _name = "GeneralRecord";
}

void GeneralRecordVitek2Compact::initFields()
{
   _bciFields.push_back(Record::createBciField("mt", "MessageType"));
   _bciFields.push_back(Record::createBciField("ii", "InstrumentCode"));
   _bciFields.push_back(Record::createBciField("is", "InstrumentSN"));
   _bciFields.push_back(Record::createBciField("it", "TestGroupCode"));
}

} // namespace bci
} // namespace tbs