#include <string>
#include "tobasalis/bci/patientrecord.h"

namespace tbs {
namespace bci {

PatientRecord::PatientRecord(const std::string& bciString)
   : Record(bciString)
{
   setRecordType(RecordType::Patient);
   initFields();
   _totalField = (int)_bciFields.size(); // 40
}

void PatientRecord::initFields()
{
   _bciFields.push_back(Record::createBciField("pi", "PatientID"));                      // pi
   _bciFields.push_back(Record::createBciField("pv", "PatientAltID"));                   // pv
   _bciFields.push_back(Record::createBciField("pn", "PatientName"));                    // pn
   _bciFields.push_back(Record::createBciField("pb", "PatientBirthDate"));               // pb
   _bciFields.push_back(Record::createBciField("ps", "PatientSex"));                     // ps
   _bciFields.push_back(Record::createBciField("pl", "PatientLocationCode"));            // pl
   _bciFields.push_back(Record::createBciField("p2", "PatientLocationText"));            // p2
   _bciFields.push_back(Record::createBciField("px", "PatientHospitalServiceCode"));     // px
   _bciFields.push_back(Record::createBciField("p3", "PatientHospitalServiceText"));     // p3
   _bciFields.push_back(Record::createBciField("po", "PatientAdmittingDiagnosisCode"));  // po
   _bciFields.push_back(Record::createBciField("p4", "PatientAdmittingDiagnosisText"));  // p4
   _bciFields.push_back(Record::createBciField("pp", "PatientPrimaryPhysicianCode"));    // pp
   _bciFields.push_back(Record::createBciField("p5", "PatientPrimaryPhysicianText"));    // p5
   _bciFields.push_back(Record::createBciField("pa", "PatientAdmissionDate"));           // pa
   _bciFields.push_back(Record::createBciField("pd", "PatientDosageGroupCode"));         // pd
   _bciFields.push_back(Record::createBciField("p6", "PatientDosageGroupText"));         // p6
   _bciFields.push_back(Record::createBciField("pc", "PatientCommentCode"));             // pc
   _bciFields.push_back(Record::createBciField("pt", "PatientCommentText"));             // pt
   _bciFields.push_back(Record::createBciField("w1", "PatientWildField1"));              // w1
   _bciFields.push_back(Record::createBciField("w2", "PatientWildField2"));              // w2
   _bciFields.push_back(Record::createBciField("w3", "PatientWildField3"));              // w3
   _bciFields.push_back(Record::createBciField("w4", "PatientWildField4"));              // w4
   _bciFields.push_back(Record::createBciField("w5", "PatientWildField5"));              // w5
   _bciFields.push_back(Record::createBciField("w6", "PatientWildField6"));              // w6
   _bciFields.push_back(Record::createBciField("w7", "PatientWildField7"));              // w7
   _bciFields.push_back(Record::createBciField("w8", "PatientWildField8"));              // w8
   _bciFields.push_back(Record::createBciField("ua", "PatientUserField1Code"));          // ua
   _bciFields.push_back(Record::createBciField("ub", "PatientUserField1Text"));          // ub
   _bciFields.push_back(Record::createBciField("uc", "PatientUserField2Code"));          // uc
   _bciFields.push_back(Record::createBciField("ud", "PatientUserField2Text"));          // ud
   _bciFields.push_back(Record::createBciField("ue", "PatientUserField3Code"));          // ue
   _bciFields.push_back(Record::createBciField("uf", "PatientUserField3Text"));          // uf
   _bciFields.push_back(Record::createBciField("u1", "PatientUserField4Code"));          // u1
   _bciFields.push_back(Record::createBciField("u2", "PatientUserField4Text"));          // u2
   _bciFields.push_back(Record::createBciField("u3", "PatientUserField5Code"));          // u3
   _bciFields.push_back(Record::createBciField("u4", "PatientUserField5Text"));          // u4
   _bciFields.push_back(Record::createBciField("p7", "PhysicianAddressLine1"));          // p7
   _bciFields.push_back(Record::createBciField("p8", "PhysicianAddressLine2"));          // p8
   _bciFields.push_back(Record::createBciField("p9", "PhysicianAddressLine3"));          // p9
   _bciFields.push_back(Record::createBciField("p0", "PhysicianAddressLine4"));          // p0
}

} // namespace bci
} // namespace tbs