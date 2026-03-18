#pragma once

#include "tobasalis/bci/record.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * SusceptibilityResultRecord
 */
class SusceptibilityResultRecord
   : public Record
{
protected:
   virtual void initFields();

public:
   SusceptibilityResultRecord(const std::string& bciString);
   SusceptibilityResultRecord() = default;
   ~SusceptibilityResultRecord() = default;

   std::string resultSeparator() { return getFieldValue("ra"); }           // ra
   std::string carSuppressionIndicator() { return getFieldValue("ar"); }   // ar
   std::string deducedDrug() { return getFieldValue("ad"); }               // ad
   std::string drugCode() { return getFieldValue("a1"); }                  // a1
   std::string drugName() { return getFieldValue("a2"); }                  // a2
   std::string finalMIC() { return getFieldValue("a3"); }                  // a3
   std::string finalResult() { return getFieldValue("a4"); }               // a4
   std::string dosageColumn1() { return getFieldValue("a5"); }             // a5
   std::string dosageColumn2() { return getFieldValue("a6"); }             // a6
   std::string dosageColumn3() { return getFieldValue("a7"); }             // a7
   std::string note() { return getFieldValue("an"); }                      // an

   void setResultSeparator(const std::string& val) { setFieldValue("ra", val); }           // ra
   void setCarSuppressionIndicator(const std::string& val) { setFieldValue("ar", val); }   // ar
   void setDeducedDrug(const std::string& val) { setFieldValue("ad", val); }               // ad
   void setDrugCode(const std::string& val) { setFieldValue("a1", val); }                  // a1
   void setDrugName(const std::string& val) { setFieldValue("a2", val); }                  // a2
   void setFinalMIC(const std::string& val) { setFieldValue("a3", val); }                  // a3
   void setFinalResult(const std::string& val) { setFieldValue("a4", val); }               // a4
   void setDosageColumn1(const std::string& val) { setFieldValue("a5", val); }             // a5
   void setDosageColumn2(const std::string& val) { setFieldValue("a6", val); }             // a6
   void setDosageColumn3(const std::string& val) { setFieldValue("a7", val); }             // a7
   void setNote(const std::string& val) { setFieldValue("an", val); }                      // an
};

} // namespace bci
} // namespace tbs 