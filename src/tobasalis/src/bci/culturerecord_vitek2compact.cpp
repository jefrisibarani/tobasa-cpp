#include <string>
#include "tobasalis/bci/culturerecord_vitek2compact.h"

namespace tbs {
namespace bci {

CultureRecordVitek2Compact::CultureRecordVitek2Compact(const std::string& bciString)
   : Record(bciString)
{
   setRecordType(RecordType::Culture);
   initFields();
   _totalField = (int)_bciFields.size();
   _name = "CultureRecord";
}

void CultureRecordVitek2Compact::initFields()
{
   _bciFields.push_back(Record::createBciField("ci", "LabID"));           // ci
   _bciFields.push_back(Record::createBciField("c0", "LabIDSystemCode")); // c0
   _bciFields.push_back(Record::createBciField("ct", "CultureTypeCode")); // ct
   _bciFields.push_back(Record::createBciField("cn", "CultureTypeName")); // cn
}

} // namespace bci
} // namespace tbs