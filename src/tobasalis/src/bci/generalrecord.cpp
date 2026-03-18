#include <string>
#include "tobasalis/bci/generalrecord.h"

namespace tbs {
namespace bci {

GeneralRecord::GeneralRecord(const std::string& bciString)
   : Record(bciString)
{
   setRecordType(RecordType::General);
   initFields();
   _totalField = (int)_bciFields.size(); // 5
}

void GeneralRecord::initFields()
{
   _bciFields.push_back(Record::createBciField("mt", "MessageType"));
   _bciFields.push_back(Record::createBciField("id", "InstrumentID"));
   _bciFields.push_back(Record::createBciField("ii", "InstrumentCode"));
   _bciFields.push_back(Record::createBciField("is", "InstrumentSN"));
   _bciFields.push_back(Record::createBciField("it", "TestGroupCode"));
}

} // namespace bci
} // namespace tbs