#pragma once

#include "tobasalis/bci/record.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * TestRecordVitek2Compact
 */
class TestRecordVitek2Compact
   : public Record
{
protected:
   virtual void initFields();

public:
   TestRecordVitek2Compact(const std::string& bciString = "");
   ~TestRecordVitek2Compact() = default;

   std::string testSeparator() { return getFieldValue("ta"); }                // ta
   // CardType
   std::string testTypeCode() { return getFieldValue("rt"); }                 // rt
   std::string isolateSystemCode() { return getFieldValue("rr"); }            // rr
   //std::string testStatusCode() { return getFieldValue("t4"); }             // t4
   //std::string testStatusName() { return getFieldValue("t5"); }             // t5

   //std::string isolateInitialReadingDate() { return getFieldValue("r1"); }  // r1
   //std::string isolateInitialReadingTime() { return getFieldValue("r2"); }  // r2
   //std::string isolateFinalCallDate() { return getFieldValue("r3"); }       // r3
   //std::string isolateFinalCallTime() { return getFieldValue("r4"); }       // r4
   //std::string cardDataSeparator() { return getFieldValue("ts"); }          // ts
   //std::string cardTypeCode() { return getFieldValue("tu"); }               // tu
   //std::string cardBarCode() { return getFieldValue("tp"); }                // tp
   //std::string cardLotNumber() { return getFieldValue("tg"); }              // tg

   //std::string cardExpirationDate() { return getFieldValue("te"); }         // te
   //std::string cardInitialReading Date() { return getFieldValue("th"); }    // th
   //std::string cardInitialReading Time() { return getFieldValue("tk"); }    // tk
   //std::string cardFinalCallDate() { return getFieldValue("td"); }          // td
   //std::string cardFinalCallTime() { return getFieldValue("tm"); }          // tm
   //std::string cardSetupTechCode() { return getFieldValue("tq"); }          // tq
   //std::string cardSetupTechName() { return getFieldValue("tr"); }          // tr
   //std::string userCommentCode() { return getFieldValue("nc"); }            // nc
   //std::string userComment() { return getFieldValue("nd"); }                // nd

   /*** ID TEST ***/
   std::string isolateNumber() { return getFieldValue("t1"); }                // t1
   std::string finalOrganismCode() { return getFieldValue("o1"); }            // o1
   std::string finalOrganismName() { return getFieldValue("o2"); }            // o2
   std::string finalBionumber() { return getFieldValue("o3"); }               // o3
   //std::string organismQuantityCode() { return getFieldValue("o4"); }       // o4
   //std::string organismQuantityName() { return getFieldValue("o5"); }       // o5
   std::string percentProbability() { return getFieldValue("o9"); }           // o9
   //std::string idConfidenceName() { return getFieldValue("oc"); }           // oc
   //std::string aesConfidenceName() { return getFieldValue("ac"); }          // ac

   void setTestSeparator(const std::string& val) { setFieldValue("ta", val); }               // ta
   void setTestTypeCode(const std::string& val) { setFieldValue("rt", val); }                // rt
   void setIsolateSystemCode(const std::string& val) { setFieldValue("rr", val); }           // rr
   //void setTestStatusCode(const std::string& val) { setFieldValue("t4",val); }             // t4
   //void setTestStatusName(const std::string& val) { setFieldValue("t5",val); }             // t5

   //void setIsolateInitialReadingDate(const std::string& val) { setFieldValue("r1",val); }  // r1
   //void setIsolateInitialReadingTime(const std::string& val) { setFieldValue("r2",val); }  // r2
   //void setIsolateFinalCallDate(const std::string& val) { setFieldValue("r3",val); }       // r3
   //void setIsolateFinalCallTime(const std::string& val) { setFieldValue("r4",val); }       // r4
   //void setCardDataSeparator(const std::string& val) { setFieldValue("ts",val); }          // ts
   //void setCardTypeCode(const std::string& val) { setFieldValue("tu",val); }               // tu
   //void setCardBarCode(const std::string& val) { setFieldValue("tp",val); }                // tp
   //void setCardLotNumber(const std::string& val) { setFieldValue("tg",val); }              // tg

   //void setCardExpirationDate(const std::string& val) { setFieldValue("te",val); }         // te
   //void setCardInitialReading Date(const std::string& val) { setFieldValue("th",val); }    // th
   //void setCardInitialReading Time(const std::string& val) { setFieldValue("tk",val); }    // tk
   //void setCardFinalCallDate(const std::string& val) { setFieldValue("td",val); }          // td
   //void setCardFinalCallTime(const std::string& val) { setFieldValue("tm",val); }          // tm
   //void setCardSetupTechCode(const std::string& val) { setFieldValue("tq",val); }          // tq
   //void setCardSetupTechName(const std::string& val) { setFieldValue("tr",val); }          // tr
   //void setUserCommentCode(const std::string& val) { setFieldValue("nc",val); }            // nc
   //void setUserComment(const std::string& val) { setFieldValue("nd",val); }                // nd

   /*** ID TEST ***/
   void setIsolateNumber(const std::string& val) { setFieldValue("t1", val); }               // t1
   void setFinalOrganismCode(const std::string& val) { setFieldValue("o1", val); }           // o1
   void setFinalOrganismName(const std::string& val) { setFieldValue("o2", val); }           // o2
   void setFinalBionumber(const std::string& val) { setFieldValue("o3", val); }              // o3
   //void setOrganismQuantityCode(const std::string& val) { setFieldValue("o4",val); }       // o4
   //void setOrganismQuantityName(const std::string& val) { setFieldValue("o5",val); }       // o5
   void setPercentProbability(const std::string& val) { setFieldValue("o9", val); }          // o9
   //void setIDConfidenceName(const std::string& val) { setFieldValue("oc",val); }           // oc
   //void setAESConfidenceName(const std::string& val) { setFieldValue("ac",val); }          // ac
};

} // namespace bci
} // namespace tbs 