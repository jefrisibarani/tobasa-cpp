#pragma once

#include "tobasalis/bci/record.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * PatientRecord
 */
class PatientRecord
   : public Record
{
protected:
   virtual void initFields();

public:
   PatientRecord(const std::string& bciString = "");
   ~PatientRecord() = default;

   std::string patientID() { return getFieldValue("pi"); }                     // pi
   std::string patientAltID() { return getFieldValue("pv"); }                  // pv
   std::string patientName() { return getFieldValue("pn"); }                   // pn
   std::string patientBirthDate() { return getFieldValue("pb"); }              // pb
   std::string patientSex() { return getFieldValue("ps"); }                    // ps
   std::string patientLocationCode() { return getFieldValue("pl"); }           // pl
   std::string patientLocationText() { return getFieldValue("p2"); }           // p2
   std::string patientHospitalServiceCode() { return getFieldValue("px"); }    // px
   std::string patientHospitalServiceText() { return getFieldValue("p3"); }    // p3
   std::string patientAdmittingDiagnosisCode() { return getFieldValue("po"); } // po
   std::string patientAdmittingDiagnosisText() { return getFieldValue("p4"); } // p4
   std::string patientPrimaryPhysicianCode() { return getFieldValue("pp"); }   // pp
   std::string patientPrimaryPhysicianText() { return getFieldValue("p5"); }   // p5
   std::string patientAdmissionDate() { return getFieldValue("pa"); }          // pa
   std::string patientDosageGroupCode() { return getFieldValue("pd"); }        // pd
   std::string patientDosageGroupText() { return getFieldValue("p6"); }        // p6
   std::string patientCommentCode() { return getFieldValue("pc"); }            // pc
   std::string patientCommentText() { return getFieldValue("pt"); }            // pt
   std::string patientWildField1() { return getFieldValue("w1"); }             // w1
   std::string patientWildField2() { return getFieldValue("w2"); }             // w2
   std::string patientWildField3() { return getFieldValue("w3"); }             // w3
   std::string patientWildField4() { return getFieldValue("w4"); }             // w4
   std::string patientWildField5() { return getFieldValue("w5"); }             // w5
   std::string patientWildField6() { return getFieldValue("w6"); }             // w6
   std::string patientWildField7() { return getFieldValue("w7"); }             // w7
   std::string patientWildField8() { return getFieldValue("w8"); }             // w8
   std::string patientUserField1Code() { return getFieldValue("ua"); }         // ua
   std::string patientUserField1Text() { return getFieldValue("ub"); }         // ub
   std::string patientUserField2Code() { return getFieldValue("uc"); }         // uc
   std::string patientUserField2Text() { return getFieldValue("ud"); }         // ud
   std::string patientUserField3Code() { return getFieldValue("ue"); }         // ue
   std::string patientUserField3Text() { return getFieldValue("uf"); }         // uf
   std::string patientUserField4Code() { return getFieldValue("u1"); }         // u1
   std::string patientUserField4Text() { return getFieldValue("u2"); }         // u2
   std::string patientUserField5Code() { return getFieldValue("u3"); }         // u3
   std::string patientUserField5Text() { return getFieldValue("u4"); }         // u4
   std::string physicianAddressLine1() { return getFieldValue("p7"); }         // p7
   std::string physicianAddressLine2() { return getFieldValue("p8"); }         // p8
   std::string physicianAddressLine3() { return getFieldValue("p9"); }         // p9
   std::string physicianAddressLine4() { return getFieldValue("p0"); }         // p0

   void setPatientID(const std::string& val) { setFieldValue("pi", val); }                     // pi
   void setPatientAltID(const std::string& val) { setFieldValue("pv", val); }                  // pv
   void setPatientName(const std::string& val) { setFieldValue("pn", val); }                   // pn
   void setPatientBirthDate(const std::string& val) { setFieldValue("pb", val); }              // pb
   void setPatientSex(const std::string& val) { setFieldValue("ps", val); }                    // ps
   void setPatientLocationCode(const std::string& val) { setFieldValue("p1", val); }           // pl
   void setPatientLocationText(const std::string& val) { setFieldValue("p2", val); }           // p2
   void setPatientHospitalServiceCode(const std::string& val) { setFieldValue("px", val); }    // px
   void setPatientHospitalServiceText(const std::string& val) { setFieldValue("p3", val); }    // p3
   void setPatientAdmittingDiagnosisCode(const std::string& val) { setFieldValue("po", val); } // po
   void setPatientAdmittingDiagnosisText(const std::string& val) { setFieldValue("p4", val); } // p4
   void setPatientPrimaryPhysicianCode(const std::string& val) { setFieldValue("pp", val); }   // pp
   void setPatientPrimaryPhysicianText(const std::string& val) { setFieldValue("p5", val); }   // p5
   void setPatientAdmissionDate(const std::string& val) { setFieldValue("pa", val); }          // pa
   void setPatientDosageGroupCode(const std::string& val) { setFieldValue("pd", val); }        // pd
   void setPatientDosageGroupText(const std::string& val) { setFieldValue("p6", val); }        // p6
   void setPatientCommentCode(const std::string& val) { setFieldValue("pc", val); }            // pc
   void setPatientCommentText(const std::string& val) { setFieldValue("pt", val); }            // pt
   void setPatientWildField1(const std::string& val) { setFieldValue("w1", val); }             // w1
   void setPatientWildField2(const std::string& val) { setFieldValue("w2", val); }             // w2
   void setPatientWildField3(const std::string& val) { setFieldValue("w3", val); }             // w3
   void setPatientWildField4(const std::string& val) { setFieldValue("w4", val); }             // w4
   void setPatientWildField5(const std::string& val) { setFieldValue("w5", val); }             // w5
   void setPatientWildField6(const std::string& val) { setFieldValue("w6", val); }             // w6
   void setPatientWildField7(const std::string& val) { setFieldValue("w7", val); }             // w7
   void setPatientWildField8(const std::string& val) { setFieldValue("w8", val); }             // w8
   void setPatientUserField1Code(const std::string& val) { setFieldValue("ua", val); }         // ua
   void setPatientUserField1Text(const std::string& val) { setFieldValue("ub", val); }         // ub
   void setPatientUserField2Code(const std::string& val) { setFieldValue("uc", val); }         // uc
   void setPatientUserField2Text(const std::string& val) { setFieldValue("ud", val); }         // ud
   void setPatientUserField3Code(const std::string& val) { setFieldValue("ue", val); }         // ue
   void setPatientUserField3Text(const std::string& val) { setFieldValue("uf", val); }         // uf
   void setPatientUserField4Code(const std::string& val) { setFieldValue("u1", val); }         // u1
   void setPatientUserField4Text(const std::string& val) { setFieldValue("u2", val); }         // u2
   void setPatientUserField5Code(const std::string& val) { setFieldValue("u3", val); }         // u3
   void setPatientUserField5Text(const std::string& val) { setFieldValue("u4", val); }         // u4
   void setPhysicianAddressLine1(const std::string& val) { setFieldValue("p7", val); }         // p7
   void setPhysicianAddressLine2(const std::string& val) { setFieldValue("p8", val); }         // p8
   void setPhysicianAddressLine3(const std::string& val) { setFieldValue("p9", val); }         // p9
   void setPhysicianAddressLine4(const std::string& val) { setFieldValue("p0", val); }         // p0
};

} // namespace bci
} // namespace tbs 