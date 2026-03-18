#include <string>
#include "tobasalis/bci/suscepresultrecord_vitek2compact.h"

namespace tbs {
namespace bci {

SusceptibilityResultRecordVitek2Compact::SusceptibilityResultRecordVitek2Compact(const std::string& bciString)
   : Record(bciString)
{
   setRecordType(RecordType::Result);
   initFields();
   _totalField = (int)_bciFields.size();
   _name = "SusceptibilityResultRecord";
}

void SusceptibilityResultRecordVitek2Compact::initFields()
{
   _bciFields.push_back(Record::createBciField("ra", "ResultSeparator"));                   // ra
   _bciFields.push_back(Record::createBciField("ar", "SuppressedAntibioticFlag"));          // ar
   _bciFields.push_back(Record::createBciField("ad", "DeducedAntibioticFlag"));             // ad
   //_bciFields.push_back( Record::createBciField("ae","DisabledWithCommentAntibioticFlag") ); // ae
   _bciFields.push_back(Record::createBciField("a1", "AntibioticCode"));                    // a1
   _bciFields.push_back(Record::createBciField("a2", "AntibioticName"));                    // a2
   _bciFields.push_back(Record::createBciField("at", "SuppressedMICFlag"));                 // at
   _bciFields.push_back(Record::createBciField("a3", "ResultMIC"));                         // a3
   _bciFields.push_back(Record::createBciField("a4", "FinalInterpretation"));               // a4
   _bciFields.push_back(Record::createBciField("an", "NonExpertisedInterpretation"));       // an

   //_bciFields.push_back( Record::createBciField("zz","ResultEnd") );     // zz
}

} // namespace bci
} // namespace tbs