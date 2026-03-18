#include <string>
#include "tobasalis/bci/suscepresultrecord.h"

namespace tbs {
namespace bci {

SusceptibilityResultRecord::SusceptibilityResultRecord(const std::string& bciString)
   : Record(bciString)
{
   setRecordType(RecordType::Result);
   initFields();
   _totalField = (int)_bciFields.size(); // 11
}

void SusceptibilityResultRecord::initFields()
{
   _bciFields.push_back(Record::createBciField("ra", "ResultSeparator"));          // ra
   _bciFields.push_back(Record::createBciField("ar", "CarSuppressionIndicator"));  // ar
   _bciFields.push_back(Record::createBciField("ad", "DeducedDrug"));              // ad
   _bciFields.push_back(Record::createBciField("a1", "DrugCode"));                 // a1
   _bciFields.push_back(Record::createBciField("a2", "DrugName"));                 // a2
   _bciFields.push_back(Record::createBciField("a3", "FinalMIC"));                 // a3
   _bciFields.push_back(Record::createBciField("a4", "FinalResult"));              // a4
   _bciFields.push_back(Record::createBciField("a5", "DosageColumn1"));            // a5
   _bciFields.push_back(Record::createBciField("a6", "DosageColumn2"));            // a6
   _bciFields.push_back(Record::createBciField("a7", "DosageColumn3"));            // a7
   _bciFields.push_back(Record::createBciField("an", "Note"));                     // an
}

} // namespace bci
} // namespace tbs