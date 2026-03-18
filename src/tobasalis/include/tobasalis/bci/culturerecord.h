#pragma once

#include "tobasalis/bci/record.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * CultureRecord
 */
class CultureRecord
   : public Record
{
protected:
   virtual void initFields();

public:
   CultureRecord(const std::string& bciString = "");
   ~CultureRecord() = default;

   std::string examinationID() { return getFieldValue("ci"); }             // ci
   std::string examinationNumber() { return getFieldValue("c0"); }         // c0
   std::string cultureTypeCode() { return getFieldValue("ct"); }           // ct
   std::string cultureTypeText() { return getFieldValue("cn"); }           // cn
   std::string cultureCompletionDate() { return getFieldValue("c1"); }     // c1
   std::string cultureCompletionTime() { return getFieldValue("c2"); }     // c2
   std::string cultureStatusCode() { return getFieldValue("c3"); }         // c3
   std::string cultureStatusText() { return getFieldValue("c4"); }         // c4
   std::string cultureWildField1() { return getFieldValue("x1"); }         // x1
   std::string cultureWildField2() { return getFieldValue("x2"); }         // x2
   std::string cultureCommandCode() { return getFieldValue("na"); }        // na
   std::string cultureCommandText() { return getFieldValue("nb"); }        // nb

   void setExaminationID(const std::string& val) { setFieldValue("ci", val); }         // ci
   void setExaminationNumber(const std::string& val) { setFieldValue("c0", val); }     // c0
   void setCultureTypeCode(const std::string& val) { setFieldValue("ct", val); }       // ct
   void setCultureTypeText(const std::string& val) { setFieldValue("cn", val); }       // cn
   void setCultureCompletionDate(const std::string& val) { setFieldValue("c1", val); } // c1
   void setCultureCompletionTime(const std::string& val) { setFieldValue("c2", val); } // c2
   void setCultureStatusCode(const std::string& val) { setFieldValue("c3", val); }     // c3
   void setCultureStatusText(const std::string& val) { setFieldValue("c4", val); }     // c4
   void setCultureWildField1(const std::string& val) { setFieldValue("x1", val); }     // x1
   void setCultureWildField2(const std::string& val) { setFieldValue("x2", val); }     // x2
   void setCultureCommandCode(const std::string& val) { setFieldValue("na", val); }    // na
   void setCultureCommandText(const std::string& val) { setFieldValue("nb", val); }    // nb
};

} // namespace bci
} // namespace tbs 