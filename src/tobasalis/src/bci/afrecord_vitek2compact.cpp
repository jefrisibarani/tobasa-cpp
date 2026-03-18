#include <string>
#include "tobasalis/bci/afrecord_vitek2compact.h"

namespace tbs {
namespace bci {

AfRecordVitek2Compact::AfRecordVitek2Compact(const std::string& bciString)
   : Record(bciString)
{
   setRecordType(RecordType::Af);
   initFields();
   _totalField = (int)_bciFields.size();
   _name = "AfRecord";
}

void AfRecordVitek2Compact::initFields()
{
   _bciFields.push_back(Record::createBciField("af", "AntibioticFamilyName"));
}

// -------------------------------------------------------

ApRecordVitek2Compact::ApRecordVitek2Compact(const std::string& bciString)
   : Record(bciString)
{
   setRecordType(RecordType::Ap);
   initFields();
   _totalField = (int)_bciFields.size();
   _name = "ApRecord";
}

void ApRecordVitek2Compact::initFields()
{
   _bciFields.push_back(Record::createBciField("ap", "PhenotypeNames"));
}

} // namespace bci
} // namespace tbs
