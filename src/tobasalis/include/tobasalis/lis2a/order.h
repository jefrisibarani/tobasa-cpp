#pragma once

#include "tobasalis/lis2a/subuniversaltest.h"

namespace tbs {
namespace lis2a {

/** \ingroup LIS
 * OrderPriority
 */
enum class OrderPriority
{
   Stat         = 'S',
   ASAP         = 'A',
   Routine      = 'R',
   CallBack     = 'C',
   PreOperative = 'P',
   None
};

/** \ingroup LIS
 * OrderActionCode
 */
enum class OrderActionCode
{
   Cancel      = 'C',   //
   Add         = 'A',   //
   New         = 'N',   //
   Pending     = 'P',   //
   Reserved    = 'L',   //
   InProcess   = 'X',   //
   QC          = 'Q',   //
   Pooled      = 'D',   //
   None
};

/** \ingroup LIS
 * OrderReportType
 */
enum class OrderReportType
{
   Order       = 'O',
   Correction  = 'C',
   Preliminary = 'P',
   Final       = 'F',
   Cancelled   = 'X',
   Pending     = 'I',
   NoOrder     = 'Y',
   NoRecord    = 'Z',
   Response    = 'Q',
   None
};

/** \ingroup LIS
 * OrderRecord.
 * CLSI LIS02-A2 (formerly ASTM 1394-97) Test Order Record
 */
class OrderRecord
   : public Record
{
public:
   OrderRecord(const std::string& lisString="");
   ~OrderRecord() = default;

   std::string recordTypeID() { return getFieldValue(1); }                   // 1
   int sequenceNumber();// { return std::stoi(getFieldValue(2)); }           // 2
   std::string specimenID() { return getFieldValue(3); }                     // 3
   std::string analyzerSpecimenID() { return getFieldValue(4); }             // 4
   UniversalTestID& testID();                                                // 5
   std::string priority() { return getFieldValue(6); }                       // 6
   OrderPriority priorityEnum();                                             // 6
   std::string orderedDateTime() { return getFieldValue(7); }                // 7
   std::string specimenCollectionDateTime() { return getFieldValue(8); }     // 8
   std::string collectionEndTime() { return getFieldValue(9); }              // 9
   std::string collectionVolume() { return getFieldValue(10); }              // 10
   std::string collectorID() { return getFieldValue(11); }                   // 11
   OrderActionCode actionCodeEnum();                                         // 12
   std::string actionCode() { return getFieldValue(12); }                    // 12
   std::string dangerCode() { return getFieldValue(13); }                    // 13
   std::string relevantClinicalInfo() { return getFieldValue(14); }          // 14
   std::string dateTimeSpecimenReceived() { return getFieldValue(15); }      // 15
   std::string specimenDescriptor() { return getFieldValue(16); }            // 16
   std::string orderingPhysician() { return getFieldValue(17); }             // 17
   std::string physicianPoneNumber() { return getFieldValue(18); }           // 18
   std::string userField1() { return getFieldValue(19); }                    // 19
   std::string userField2() { return getFieldValue(20); }                    // 20
   std::string laboratoryField1() { return getFieldValue(21); }              // 21
   std::string laboratoryField2() { return getFieldValue(22); }              // 22
   std::string lastModified() { return getFieldValue(23); }                  // 23
   std::string analyzerChargetoIS() { return getFieldValue(24); }            // 24
   std::string instrumentSectionID() { return getFieldValue(25); }           // 25
   std::string reportType() { return getFieldValue(26); }                    // 26
   OrderReportType reportTypeEnum();                                         // 26
   std::string reservedFiedld() { return getFieldValue(27); }                // 27
   std::string locationOfSpecimenColl() { return getFieldValue(28); }        // 28
   std::string nosocomialInfectionFlag() { return getFieldValue(29); }       // 29
   std::string specimenService() { return getFieldValue(30); }               // 30
   std::string specimenInstitution() { return getFieldValue(31); }           // 31


   void setRecordTypeID(const std::string& val) { setFieldValue(1, val); }                // 1
   void setSequenceNumber(int number) { setFieldValue(2, std::to_string(number)); }       // 2
   void setSpecimenID(const std::string& val) { setFieldValue(3, val); }                  // 3
   void setAnalyzerSpecimenID(const std::string& val) { setFieldValue(4, val); }          // 4
   void setTestID(UniversalTestID testID);                                                // 5
   void setPriority(const std::string& val) { setFieldValue(6, val); }                    // 6
   void setPriority(OrderPriority priority);                                              // 6
   void setOrderedDateTime(const std::string& val) { setFieldValue(7, val); }             // 7
   void setSpecimenCollectionDateTime(const std::string& val) { setFieldValue(8, val); }  // 8
   void setCollectionEndTime(const std::string& val) { setFieldValue(9, val); }           // 9
   void setCollectionVolume(const std::string& val) { setFieldValue(10, val); }           // 10
   void setCollectorID(const std::string& val) { setFieldValue(11, val); }                // 11
   void setActionCode(const std::string& val) { setFieldValue(12, val); }                 // 12
   void setActionCode(OrderActionCode code);                                              // 12
   void setDangerCode(const std::string& val) { setFieldValue(13, val); }                 // 13
   void setRelevantClinicalInfo(const std::string& val) { setFieldValue(14, val); }       // 14
   void setDateTimeSpecimenReceived(const std::string& val) { setFieldValue(15, val); }   // 15
   void setSpecimenDescriptor(const std::string& val) { setFieldValue(16, val); }         // 16
   void setOrderingPhysician(const std::string& val) { setFieldValue(17, val); }          // 17
   void setPhysicianPoneNumber(const std::string& val) { setFieldValue(18, val); }        // 18
   void setUserField1(const std::string& val) { setFieldValue(19, val); }                 // 19
   void setUserField2(const std::string& val) { setFieldValue(20, val); }                 // 20
   void setLaboratoryField1(const std::string& val) { setFieldValue(21, val); }           // 21
   void setLaboratoryField2(const std::string& val) { setFieldValue(22, val); }           // 22
   void setLastModified(const std::string& val) { setFieldValue(23, val); }               // 23
   void setAnalyzerChargetoIS(const std::string& val) { setFieldValue(24, val); }         // 24
   void setInstrumentSectionID(const std::string& val) { setFieldValue(25, val); }        // 25

   void setReportType(const std::string& val) { setFieldValue(26, val); }                 // 26
   void setReportType(OrderReportType type);                                              // 26
   void setReservedFiedld(const std::string& val) {setFieldValue(27, val); }              // 27
   void setLocationOfSpecimenColl(const std::string& val) { setFieldValue(28, val); }     // 28
   void setNosocomialInfectionFlag(const std::string& val) { setFieldValue(29, val); }    // 29
   void setSpecimenService(const std::string& val) { setFieldValue(30, val); }            // 30
   void setSpecimenInstitution(const std::string& val) { setFieldValue(31, val); }        // 31

private:
   UniversalTestID _univSubRecord;

protected:
   virtual void initFields();
};

} // namespace lis2a
} // namespace tbs