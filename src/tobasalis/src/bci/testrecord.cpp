#include <string>
#include "tobasalis/bci/testrecord.h"

namespace tbs {
namespace bci {

TestRecord::TestRecord(const std::string& bciString)
   : Record(bciString)
{
   setRecordType(RecordType::Test);
   initFields();
   _totalField = (int)_bciFields.size(); // 44
}

void TestRecord::initFields()
{
   //_bciFields.push_back( Record::createBciField("ci","ExaminationID") );           // ci
   _bciFields.push_back(Record::createBciField("ta", "TestSeparator"));              // ta
   _bciFields.push_back(Record::createBciField("rt", "TestTypeCode"));               // rt
   _bciFields.push_back(Record::createBciField("rn", "TestTypeName"));               // rn
   _bciFields.push_back(Record::createBciField("rr", "RelativeTestNumber"));         // rr
   _bciFields.push_back(Record::createBciField("ti", "InstrumentSystemCode"));       // ti
   _bciFields.push_back(Record::createBciField("tj", "InstrumentName"));             // tj
   _bciFields.push_back(Record::createBciField("tc", "InstrumentCommentCode"));      // tc
   _bciFields.push_back(Record::createBciField("tn", "InstrumentCommentText"));      // tn
   _bciFields.push_back(Record::createBciField("t2", "TestGroupCode"));              // t2
   _bciFields.push_back(Record::createBciField("t3", "TestGroupName"));              // t3
   _bciFields.push_back(Record::createBciField("t4", "TestStatusCode"));             // t4
   _bciFields.push_back(Record::createBciField("t5", "TestStatusName"));             // t5

   _bciFields.push_back(Record::createBciField("m3", "TechnologistCode"));           // m3
   _bciFields.push_back(Record::createBciField("m4", "TechnologistName"));           // m4
   _bciFields.push_back(Record::createBciField("nc", "TestCommentCode"));            // nc
   _bciFields.push_back(Record::createBciField("nd", "TestCommentText"));            // nd
   _bciFields.push_back(Record::createBciField("y1", "TestWildText1"));              // y1
   _bciFields.push_back(Record::createBciField("y2", "TestWildText2"));              // y2
   _bciFields.push_back(Record::createBciField("y3", "TestWildText3"));              // y3
   _bciFields.push_back(Record::createBciField("y4", "TestWildText4"));              // y4

   /*** ID TEST ***/
   _bciFields.push_back(Record::createBciField("t1", "IsolateNumber"));              //t1
   _bciFields.push_back(Record::createBciField("tb", "BottleBarCode"));              //tb
   _bciFields.push_back(Record::createBciField("t6", "PreliminaryOrganismCode"));    //t6
   _bciFields.push_back(Record::createBciField("t7", "PreliminaryOrganismName"));    //t7
   _bciFields.push_back(Record::createBciField("t8", "PreliminaryModifierCode"));    //t8
   _bciFields.push_back(Record::createBciField("t9", "PreliminaryModifierName"));    //t9

   _bciFields.push_back(Record::createBciField("o1", "FinalOrganismCode"));          // o1
   _bciFields.push_back(Record::createBciField("o2", "FinalOrganismName"));          // o2
   _bciFields.push_back(Record::createBciField("o3", "FinalBionumber"));             // o3
   _bciFields.push_back(Record::createBciField("o4", "FinalModifierCode"));          // o4
   _bciFields.push_back(Record::createBciField("o5", "FinalModifierName"));          // o5
   _bciFields.push_back(Record::createBciField("o6", "OrganismGroupCode"));          // o6
   _bciFields.push_back(Record::createBciField("o7", "OrganismGroupName"));          // o7
   _bciFields.push_back(Record::createBciField("o9", "PercentProbability"));         // o9

   /*** SUSC TEST ***/
   /** Contains fields in ID TEST, minus "o3" "o9"        **/
   /** Result fields: ra,ar,ad,a1,a2,a3,a4,a5,a6,a7,an    **/
   _bciFields.push_back(Record::createBciField("tt", "TestFreeText"));               //tt
   _bciFields.push_back(Record::createBciField("af", "AntibioticFamilyName"));       //af
   _bciFields.push_back(Record::createBciField("ap", "PhenotypeNames"));             //ap

   /*** SMEAR TEST ***/
   /** Fields : tb  **/
   /** Result fields: ra,rs,rm,o4,o5

   /*** SCREEN TEST ***/
   /** Result fields: ra,o1,o2,o3,o4,o5,o6,o7,o8,o3,rh **/

   /*** BLOOD TEST ***/
   /** Fields : t1,o1,tb **/
   _bciFields.push_back(Record::createBciField("rd", "ResultDate"));                 //rd
   _bciFields.push_back(Record::createBciField("ru", "ResultTime"));                 //ru
   _bciFields.push_back(Record::createBciField("re", "ElapsedHours"));               //re
   _bciFields.push_back(Record::createBciField("rc", "InstrumentResultCode"));       //rc
   _bciFields.push_back(Record::createBciField("ri", "InstrumentResultName"));       //ri
   _bciFields.push_back(Record::createBciField("rj", "BloodTestFinalResultCode"));   //rj
   _bciFields.push_back(Record::createBciField("rk", "BloodTestFinalResultName"));   //rk
}

} // namespace bci
} // namespace tbs