#include <string>
#include "tobasalis/bci/specimenrecord_vitek2compact.h"

namespace tbs {
namespace bci {

SpecimenRecordVitek2Compact::SpecimenRecordVitek2Compact(const std::string& bciString)
   : Record(bciString)
{
   setRecordType(RecordType::Specimen);
   initFields();
   _totalField = (int)_bciFields.size();
   _name = "SpecimenRecord";
}

void SpecimenRecordVitek2Compact::initFields()
{
   _bciFields.push_back(Record::createBciField("si", "SpecimenSeparator"));                 // si
   _bciFields.push_back(Record::createBciField("s0", "SpecimenSystemCode"));                // s0
   _bciFields.push_back(Record::createBciField("ss", "SpecimenSourceCode"));                // ss
   _bciFields.push_back(Record::createBciField("s5", "SpecimenSourceName"));                // s5
   _bciFields.push_back(Record::createBciField("s1", "SpecimenCollectionDate"));            // s1
   _bciFields.push_back(Record::createBciField("s2", "SpecimenCollectionTime"));            // s2
   _bciFields.push_back(Record::createBciField("s3", "SpecimenReceiptDate"));               // s3
   _bciFields.push_back(Record::createBciField("s4", "SpecimenReceiptTime"));               // s4
   _bciFields.push_back(Record::createBciField("sc", "SpecimenCommentCode"));               // sc
   _bciFields.push_back(Record::createBciField("sn", "SpecimenCommentText"));               // sn
}

} // namespace bci
} // namespace tbs