#pragma once

#include "tobasalis/lis2a/subpatient.h"

namespace tbs {
namespace lis2a {

/** \ingroup LIS
 * PatientSex
 */
enum class PatientSex
{
   Male     = 'M',
   Female   = 'F',
   Unknown  = 'U'
};

class PatientName;

/** \ingroup LIS
 * PatientRecord
 * CLSI LIS02-A2 (formerly ASTM 1394-97) Patient Information Record
 */
class PatientRecord
   : public Record
{
public:
   PatientRecord(const std::string& lisString = "" );
   ~PatientRecord() = default;

   std::string recordTypeID()                { return getFieldValue(1); }              // 1
   int sequenceNumber()                      { return std::stoi(getFieldValue(2)); }   // 2
   std::string practiceAssignedPatientID()   { return getFieldValue(3); }              // 3
   std::string laboratoryAssignedPatientID() { return getFieldValue(4); }              // 4
   std::string patientID3()                  { return getFieldValue(5); }              // 5 
   PatientName& getPatientName();                                                      // 6
   std::string motherMaidenName()            { return getFieldValue(7); }              // 7
   std::string birthdate()                   { return getFieldValue(8); }              // 8
   PatientSex getPatientSexEnum();                                                     // 9
   std::string getPatientSex()               { return getFieldValue(9); }              // 9
   std::string patientEthnicOrigin()         { return getFieldValue(10); }             // 10
   std::string patientAddress()              { return getFieldValue(11); }             // 11
   std::string reservedField()               { return getFieldValue(12); }             // 12
   std::string patientTelephoneNumber()      { return getFieldValue(13); }             // 13
   std::string attendingPhysicianID()        { return getFieldValue(14); }             // 14
   std::string specialField1()               { return getFieldValue(15); }             // 15
   std::string specialField2()               { return getFieldValue(16); }             // 16
   std::string patientHeight()               { return getFieldValue(17); }             // 17
   std::string patientWeight()               { return getFieldValue(18); }             // 18
   std::string patientDiagnosis()            { return getFieldValue(19); }             // 19
   std::string patientMedications()          { return getFieldValue(20); }             // 20
   std::string patientDiet()                 { return getFieldValue(21); }             // 21
   std::string practiceField1()              { return getFieldValue(22); }             // 22
   std::string practiceField2()              { return getFieldValue(23); }             // 23
   std::string admissionAndDischargeDates()  { return getFieldValue(24); }             // 24
   std::string admissionStatus()             { return getFieldValue(25); }             // 25
   std::string location()                    { return getFieldValue(26); }             // 26
   std::string natureOfAltDiagCodeAndClass() { return getFieldValue(27); }             // 27
   std::string altDiagCodeAndClass()         { return getFieldValue(28); }             // 28
   std::string patientReligion()             { return getFieldValue(29); }             // 29
   std::string maritalStatus()               { return getFieldValue(30); }             // 30
   std::string isolationStatus()             { return getFieldValue(31); }             // 31
   std::string language()                    { return getFieldValue(32); }             // 32
   std::string hospitalService()             { return getFieldValue(33); }             // 33
   std::string hospitalInstitution()         { return getFieldValue(34); }             // 34
   std::string dosageCategory()              { return getFieldValue(35); }             // 35

   void setRecordTypeID(const char id)   { setFieldValue(1, std::string(1, id)); }         // 1
   void setSequenceNumber(int number)    { setFieldValue(2, std::to_string(number)); }     // 2
   void setPracticeAssignedPatientID(const std::string& val)   { setFieldValue(3, val); }  // 3
   void setLaboratoryAssignedPatientID(const std::string& val) { setFieldValue(4, val); }  // 4
   void setPatientID3(const std::string& val)            { setFieldValue(5, val); }        // 5
   void setPatientName(PatientName patient);                                               // 6
   void setMotherMaidenName(const std::string& val)      { setFieldValue(7, val); }        // 7
   void setBirthdate(const std::string& val)             { setFieldValue(8, val); }        // 8
   void setPatientSex(const std::string& val)            { setFieldValue(9, val); }        // 9
   void setPatientSex(PatientSex sex);                                                     // 9
   void setPatientEthnicOrigin(const std::string& val)   { setFieldValue(10, val); }       // 10
   void setPatientAddress(const std::string& val)        { setFieldValue(11, val); }       // 11
   void setReservedField(const std::string& val)         { setFieldValue(12, val); }       // 12
   void setPatientTelephoneNumber(const std::string& val){ setFieldValue(13, val); }       // 13
   void setAttendingPhysicianID(const std::string& val)  { setFieldValue(14, val); }       // 14
   void setSpecialField1(const std::string& val)         { setFieldValue(15, val); }       // 15
   void setSpecialField2(const std::string& val)         { setFieldValue(16, val); }       // 16
   void setPatientHeight(const std::string& val)         { setFieldValue(17, val); }       // 17
   void setPatientWeight(const std::string& val)         { setFieldValue(18, val); }       // 18
   void setPatientDiagnosis(const std::string& val)      { setFieldValue(19, val); }       // 19
   void setPatientMedications(const std::string& val)    { setFieldValue(20, val); }       // 20
   void setPatientDiet(const std::string& val)           { setFieldValue(21, val); }       // 21
   void setPracticeField1(const std::string& val)        { setFieldValue(22, val); }       // 22
   void setPracticeField2(const std::string& val)        { setFieldValue(23, val); }       // 23
   void setAdmissionAndDischargeDates(const std::string& val)  { setFieldValue(24, val); } // 24
   void setAdmissionStatus(const std::string& val)       { setFieldValue(25, val); }       // 25
   void setLocation(const std::string& val)              { setFieldValue(26, val); }       // 26
   void setNatureOfAltDiagCodeAndClass(const std::string& val) { setFieldValue(27, val); } // 27
   void setAltDiagCodeAndClass(const std::string& val)   { setFieldValue(28, val); }       // 28
   void setPatientReligion(const std::string& val)       { setFieldValue(29, val); }       // 29
   void setMaritalStatus(const std::string& val)         { setFieldValue(30, val); }       // 30
   void setIsolationStatus(const std::string& val)       { setFieldValue(31, val); }       // 31
   void setLanguage(const std::string& val)              { setFieldValue(32, val); }       // 32
   void setHospitalService(const std::string& val)       { setFieldValue(33, val); }       // 33
   void setHospitalInstitution(const std::string& val)   { setFieldValue(34, val); }       // 34
   void setDosageCategory(const std::string& val)        { setFieldValue(35, val); }       // 35

private:
   PatientName _patientSubRecord;

protected:
   virtual void initFields();
};

} // namespace lis2a
} // namespace tbs 