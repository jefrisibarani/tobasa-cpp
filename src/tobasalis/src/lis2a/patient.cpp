#include <string>
#include <tobasa/logger.h>
#include "tobasalis/lis2a/subrecord.h"
#include "tobasalis/lis2a/patient.h"

namespace tbs {
namespace lis2a {

PatientRecord::PatientRecord(const std::string& lisString)
   : Record(lisString)
{
   setRecordType(RecordType::Patient);
   _totalField = 35;
   initFields();
}

void PatientRecord::initFields()
{
   _lisFields.push_back(Record::createField(1, "RecordTypeID", "P"));
   _lisFields.push_back(Record::createField(2, "SequenceNumber"));
   _lisFields.push_back(Record::createField(3, "PracticeAssignedPatientID"));
   _lisFields.push_back(Record::createField(4, "LaboratoryAssignedPatientID"));
   _lisFields.push_back(Record::createField(5, "PatientID3"));

   LisField info6(6, "PatientName");
   info6.setValueType(FieldValueType::SubRecord);
   info6.setType(FieldType::Component);
   info6.setSubRecord(&_patientSubRecord);
   _lisFields.push_back(info6);

   _lisFields.push_back(Record::createField(7, "MotherMaidenName"));
   _lisFields.push_back(Record::createField(8, "Birthdate"));

   LisField info9(9, "PatientSex");
   info9.setValueType(FieldValueType::Enum);
   _lisFields.push_back(info9);

   _lisFields.push_back(Record::createField(10, "PatientEthnicOrigin"));
   _lisFields.push_back(Record::createField(11, "PatientAddress"));
   _lisFields.push_back(Record::createField(12, "ReservedField"));
   _lisFields.push_back(Record::createField(13, "PatientTelephoneNumber"));
   _lisFields.push_back(Record::createField(14, "AttendingPhysicianID"));
   _lisFields.push_back(Record::createField(15, "SpecialField1"));
   _lisFields.push_back(Record::createField(16, "SpecialField2"));
   _lisFields.push_back(Record::createField(17, "PatientHeight"));
   _lisFields.push_back(Record::createField(18, "PatientWeight"));
   _lisFields.push_back(Record::createField(19, "PatientDiagnosis"));
   _lisFields.push_back(Record::createField(20, "PatientMedications"));
   _lisFields.push_back(Record::createField(21, "PatientDiet"));
   _lisFields.push_back(Record::createField(22, "PracticeField1"));
   _lisFields.push_back(Record::createField(23, "PracticeField2"));
   _lisFields.push_back(Record::createField(24, "AdmissionAndDischargeDates"));
   _lisFields.push_back(Record::createField(25, "AdmissionStatus"));
   _lisFields.push_back(Record::createField(26, "Location"));
   _lisFields.push_back(Record::createField(27, "NatureOfAltDiagCodeAndClass"));
   _lisFields.push_back(Record::createField(28, "AltDiagCodeAndClass"));
   _lisFields.push_back(Record::createField(29, "PatientReligion"));
   _lisFields.push_back(Record::createField(30, "MaritalStatus"));
   _lisFields.push_back(Record::createField(31, "IsolationStatus"));
   _lisFields.push_back(Record::createField(32, "Language"));
   _lisFields.push_back(Record::createField(33, "HospitalService"));
   _lisFields.push_back(Record::createField(34, "HospitalInstitution"));
   _lisFields.push_back(Record::createField(35, "DosageCategory"));
}

PatientName& PatientRecord::getPatientName()
{
   return _patientSubRecord;
}

PatientSex PatientRecord::getPatientSexEnum()
{
   std::string value = getFieldValue(9);

   if (value == "M")
      return PatientSex::Male;
   if (value == "F")
      return PatientSex::Female;
   if (value == "U")
      return PatientSex::Unknown;

   return PatientSex::Unknown;
}

void PatientRecord::setPatientName(PatientName patient)
{
   _patientSubRecord.fromString(patient.toString());
}

void PatientRecord::setPatientSex(PatientSex sex)
{
   switch (sex)
   {
   case PatientSex::Male:
      setFieldValue(9, "M");
      break;
   case PatientSex::Female:
      setFieldValue(9, "F");
      break;
   case PatientSex::Unknown:
      setFieldValue(9, "U");
      break;
   }
}

} // namespace lis2a
} // namespace tbs