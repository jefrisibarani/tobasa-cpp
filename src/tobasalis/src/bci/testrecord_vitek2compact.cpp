#include <string>
#include "tobasalis/bci/testrecord_vitek2compact.h"

namespace tbs {
namespace bci {

TestRecordVitek2Compact::TestRecordVitek2Compact(const std::string& bciString)
   : Record(bciString)
{
   setRecordType(RecordType::Test);
   initFields();
   _totalField = (int)_bciFields.size();
   _name = "TestRecord";
}

void TestRecordVitek2Compact::initFields()
{
   _bciFields.push_back(Record::createBciField("ta", "TestSeparator"));                 // ta
   _bciFields.push_back(Record::createBciField("rt", "TestTypeCode"));                  // rt
   _bciFields.push_back(Record::createBciField("rr", "IsolateSystemCode"));             // rr
   //_bciFields.push_back( Record::createBciField("t4","TestStatusCode") );             // t4
   //_bciFields.push_back( Record::createBciField("t5","TestStatusName") );             // t5

   //_bciFields.push_back( Record::createBciField("r1","IsolateInitialReadingDate") );  // r1
   //_bciFields.push_back( Record::createBciField("r2","IsolateInitialReadingTime") );  // r2
   //_bciFields.push_back( Record::createBciField("r3","IsolateFinalCallDate") );       // r3
   //_bciFields.push_back( Record::createBciField("r4","IsolateFinalCallTime") );       // r4
   //_bciFields.push_back( Record::createBciField("ts","CardDataSeparator") );          // ts
   //_bciFields.push_back( Record::createBciField("tu","CardTypeCode") );               // tu
   //_bciFields.push_back( Record::createBciField("tp","CardBarCode") );                // tp
   //_bciFields.push_back( Record::createBciField("tg","CardLotNumber") );              // tg

   //_bciFields.push_back( Record::createBciField("te","CardExpirationDate") );         // te
   //_bciFields.push_back( Record::createBciField("th","CardInitialReadingDate") );     // th
   //_bciFields.push_back( Record::createBciField("tk","CardInitialReadingTime") );     // tk
   //_bciFields.push_back( Record::createBciField("td","CardFinalCallDate") );          // td
   //_bciFields.push_back( Record::createBciField("tm","CardFinalCallTime") );          // tm
   //_bciFields.push_back( Record::createBciField("tq","CardSetupTechCode") );          // tq
   //_bciFields.push_back( Record::createBciField("tr","CardSetupTechName") );          // tr
   //_bciFields.push_back( Record::createBciField("nc","UserCommentCode") );            // nc
   //_bciFields.push_back( Record::createBciField("nd","UserComment") );                // nd

   _bciFields.push_back(Record::createBciField("t1", "IsolateNumber"));                 // t1
   _bciFields.push_back(Record::createBciField("o1", "FinalOrganismCode"));             // o1
   _bciFields.push_back(Record::createBciField("o2", "FinalOrganismName"));             // o2
   _bciFields.push_back(Record::createBciField("o3", "FinalBionumber"));                // o3
   //_bciFields.push_back( Record::createBciField("o4","OrganismQuantityCode") );       // o4
   //_bciFields.push_back( Record::createBciField("o5","OrganismQuantityName") );       // o5
   _bciFields.push_back(Record::createBciField("o9", "PercentProbability"));            // o9
   //_bciFields.push_back( Record::createBciField("oc","IDConfidenceName") );           // oc
   //_bciFields.push_back( Record::createBciField("ac","AESConfidenceName") );          // ac
}

} // namespace bci
} // namespace tbs