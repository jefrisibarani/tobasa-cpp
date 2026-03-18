#pragma once

#include "tobasalis/bci/record.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * TestRecord
 */
class TestRecord
   : public Record
{
protected:
   virtual void initFields();

public:
   TestRecord(const std::string& bciString = "");
   ~TestRecord() = default;

   std::string testSeparator() { return getFieldValue("ta"); }              // ta
   std::string testTypeCode() { return getFieldValue("rt"); }               // rt
   std::string testTypeName() { return getFieldValue("rn"); }               // rn
   std::string relativeTestNumber() { return getFieldValue("rr"); }         // rr
   std::string instrumentSystemCode() { return getFieldValue("ti"); }       // ti
   std::string instrumentName() { return getFieldValue("tj"); }             // tj
   std::string instrumentCommentCode() { return getFieldValue("tc"); }      // tc
   std::string instrumentCommentText() { return getFieldValue("tn"); }      // tn
   std::string testGroupCode() { return getFieldValue("t2"); }              // t2
   std::string testGroupName() { return getFieldValue("t3"); }              // t3
   std::string testStatusCode() { return getFieldValue("t4"); }             // t4
   std::string testStatusName() { return getFieldValue("t5"); }             // t5

   std::string technologistCode() { return getFieldValue("m3"); }           // m3
   std::string technologistName() { return getFieldValue("m4"); }           // m4
   std::string testCommentCode() { return getFieldValue("nc"); }            // nc
   std::string testCommentText() { return getFieldValue("nd"); }            // nd
   std::string testWildText1() { return getFieldValue("y1"); }              // y1
   std::string testWildText2() { return getFieldValue("y2"); }              // y2
   std::string testWildText3() { return getFieldValue("y3"); }              // y3
   std::string testWildText4() { return getFieldValue("y4"); }              // y4

   //std::string testTerminator() { return getFieldValue("zz"); }           // zz

   // ID TEST
   std::string isolateNumber() { return getFieldValue("t1"); }              //t1
   std::string bottleBarCode() { return getFieldValue("tb"); }              //tb
   std::string preliminaryOrganismCode() { return getFieldValue("t6"); }    //t6
   std::string preliminaryOrganismName() { return getFieldValue("t7"); }    //t7
   std::string preliminaryModifierCode() { return getFieldValue("t8"); }    //t8
   std::string preliminaryModifierName() { return getFieldValue("t9"); }    //t9

   std::string finalOrganismCode() { return getFieldValue("o1"); }          // o1
   std::string finalOrganismName() { return getFieldValue("o2"); }          // o2
   std::string finalBionumber() { return getFieldValue("o3"); }             // o3
   std::string finalModifierCode() { return getFieldValue("o4"); }          // o4
   std::string finalModifierName() { return getFieldValue("o5"); }          // o5
   std::string organismGroupCode() { return getFieldValue("o6"); }          // o6
   std::string organismGroupName() { return getFieldValue("o7"); }          // o7
   std::string percentProbability() { return getFieldValue("o9"); }         // o9

   // SUSC TEST
   // Contains fields in ID TEST, minus "o3" "o9"
   // Result fields: ra,ar,ad,a1,a2,a3,a4,a5,a6,a7,an
   std::string testFreeText() { return getFieldValue("tt"); }               //tt
   std::string antibioticFamilyName() { return getFieldValue("af"); }       //af
   std::string phenotypeNames() { return getFieldValue("ap"); }             //ap

   // SMEAR TEST
   // Fields : tb
   // Result fields: ra,rs,rm,o4,o5

   // SCREEN TEST
   // Result fields: ra,o1,o2,o3,o4,o5,o6,o7,o8,o3,rh

   // BLOOD TEST
   // Fields : t1,o1,tb
   std::string resultDate() { return getFieldValue("rd"); }                                 //rd
   std::string resultTime() { return getFieldValue("ru"); }                                 //ru
   std::string elapsedHours() { return getFieldValue("re"); }                               //re
   std::string instrumentResultCode() { return getFieldValue("rc"); }                       //rc
   std::string instrumentResultName() { return getFieldValue("ri"); }                       //ri
   std::string bloodTestFinalResultCode() { return getFieldValue("rj"); }                   //rj
   std::string bloodTestFinalResultName() { return getFieldValue("rk"); }                   //rk

   void setTestSeparator(const std::string& val) { setFieldValue("ta", val); }              // ta
   void setTestTypeCode(const std::string& val) { setFieldValue("rt", val); }               // rt
   void setTestTypeName(const std::string& val) { setFieldValue("rn", val); }               // rn
   void setRelativeTestNumber(const std::string& val) { setFieldValue("rr", val); }         // rr
   void setInstrumentSystemCode(const std::string& val) { setFieldValue("ti", val); }       // ti
   void setInstrumentName(const std::string& val) { setFieldValue("tj", val); }             // tj
   void setInstrumentCommentCode(const std::string& val) { setFieldValue("tc", val); }      // tc
   void setInstrumentCommentText(const std::string& val) { setFieldValue("tn", val); }      // tn
   void setTestGroupCode(const std::string& val) { setFieldValue("t2", val); }              // t2
   void setTestGroupName(const std::string& val) { setFieldValue("t3", val); }              // t3
   void setTestStatusCode(const std::string& val) { setFieldValue("t4", val); }             // t4
   void setTestStatusName(const std::string& val) { setFieldValue("t5", val); }             // t5

   void setTechnologistCode(const std::string& val) { setFieldValue("m3", val); }           // m3
   void setTechnologistName(const std::string& val) { setFieldValue("m4", val); }           // m4
   void setTestCommentCode(const std::string& val) { setFieldValue("nc", val); }            // nc
   void setTestCommentText(const std::string& val) { setFieldValue("nd", val); }            // nd
   void setTestWildText1(const std::string& val) { setFieldValue("y1", val); }              // y1
   void setTestWildText2(const std::string& val) { setFieldValue("y2", val); }              // y2
   void setTestWildText3(const std::string& val) { setFieldValue("y3", val); }              // y3
   void setTestWildText4(const std::string& val) { setFieldValue("y4", val); }              // y4

   //void setTestTerminator(const std::string& val) { setFieldValue("zz",val); }            // y4

   // ID TEST
   void setIsolateNumber(const std::string& val) { setFieldValue("t1", val); }              // t1
   void setBottleBarCode(const std::string& val) { setFieldValue("tb", val); }              // tb
   void setPreliminaryOrganismCode(const std::string& val) { setFieldValue("t6", val); }    // t6
   void setPreliminaryOrganismName(const std::string& val) { setFieldValue("t7", val); }    // t7
   void setPreliminaryModifierCode(const std::string& val) { setFieldValue("t8", val); }    // t8
   void setPreliminaryModifierName(const std::string& val) { setFieldValue("t9", val); }    // t9

   void setFinalOrganismCode(const std::string& val) { setFieldValue("o1", val); }           // o1
   void setFinalOrganismName(const std::string& val) { setFieldValue("o2", val); }           // o2
   void setFinalBionumber(const std::string& val) { setFieldValue("o3", val); }              // o3
   void setFinalModifierCode(const std::string& val) { setFieldValue("o4", val); }           // o4
   void setFinalModifierName(const std::string& val) { setFieldValue("o5", val); }           // o5
   void setOrganismGroupCode(const std::string& val) { setFieldValue("o6", val); }           // o6
   void setOrganismGroupName(const std::string& val) { setFieldValue("o7", val); }           // o7
   void setPercentProbability(const std::string& val) { setFieldValue("o9", val); }          // o9

   // SUSC TEST
   // Contains fields in ID TEST, minus "o3" "o9"
   // Result fields: ra,ar,ad,a1,a2,a3,a4,a5,a6,a7,an
   void setTestFreeText(const std::string& val) { setFieldValue("tt", val); }                //tt
   void setAntibioticFamilyName(const std::string& val) { setFieldValue("af", val); }        //af
   void setPhenotypeNames(const std::string& val) { setFieldValue("ap", val); }              //ap

   // SMEAR TEST
   // Fields : tb
   // Result fields: ra,rs,rm,o4,o5

   // SCREEN TEST
   // Result fields: ra,o1,o2,o3,o4,o5,o6,o7,o8,o3,rh

   // BLOOD TEST
   // Fields : t1,o1,tb
   void setResultDate(const std::string& val) { setFieldValue("rd", val); }                   //rd
   void setResultTime(const std::string& val) { setFieldValue("ru", val); }                   //ru
   void setElapsedHours(const std::string& val) { setFieldValue("re", val); }                 //re
   void setInstrumentResultCode(const std::string& val) { setFieldValue("rc", val); }         //rc
   void setInstrumentResultName(const std::string& val) { setFieldValue("ri", val); }         //ri
   void setBloodTestFinalResultCode(const std::string& val) { setFieldValue("rj", val); }     //rj
   void setBloodTestFinalResultName(const std::string& val) { setFieldValue("rk", val); }     //rk
};

} // namespace bci
} // namespace tbs 