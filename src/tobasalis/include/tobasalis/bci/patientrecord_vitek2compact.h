#pragma once

#include "tobasalis/bci/record.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * PatientRecordVitek2Compact
 */
class PatientRecordVitek2Compact
   : public Record
{
protected:
   virtual void initFields();

public:
   PatientRecordVitek2Compact(const std::string& bciString);
   PatientRecordVitek2Compact() = default;
   ~PatientRecordVitek2Compact() = default;

   std::string patientID() { return getFieldValue("pi"); }                     // pi
   std::string patientAltID() { return getFieldValue("pv"); }                  // pv
   std::string patientName() { return getFieldValue("pn"); }                   // pn
   std::string patientLocationCode() { return getFieldValue("pl"); }           // pl
   std::string patientLocationName() { return getFieldValue("p2"); }           // p2
   std::string patientPhysicianCode() { return getFieldValue("pp"); }          // pp
   std::string patientPhysicianName() { return getFieldValue("p5"); }          // p5
   std::string patientCommentCode() { return getFieldValue("pc"); }            // pc
   std::string patientCommentText() { return getFieldValue("pt"); }            // pt
   std::string patientWildField1() { return getFieldValue("w1"); }             // w1

   void setPatientID(const std::string& val) { setFieldValue("pi", val); }              // pi
   void setPatientAltID(const std::string& val) { setFieldValue("pv", val); }           // pv
   void setPatientName(const std::string& val) { setFieldValue("pn", val); }            // pn
   void setPatientLocationCode(const std::string& val) { setFieldValue("p1", val); }    // pl
   void setPatientLocationName(const std::string& val) { setFieldValue("p2", val); }    // p2
   void setPatientPhysicianCode(const std::string& val) { setFieldValue("pp", val); }   // pp
   void setPatientPhysicianName(const std::string& val) { setFieldValue("p5", val); }   // p5
   void setPatientCommentCode(const std::string& val) { setFieldValue("pc", val); }     // pc
   void setPatientCommentText(const std::string& val) { setFieldValue("pt", val); }     // pt
   void setPatientWildField1(const std::string& val) { setFieldValue("w1", val); }      // w1
};

} // namespace bci
} // namespace tbs 