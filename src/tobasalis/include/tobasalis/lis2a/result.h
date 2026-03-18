#pragma once

#include <string>
#include "tobasalis/lis2a/subuniversaltest.h"

namespace tbs {
namespace lis2a {

/** \ingroup LIS
 * ResultAbnormalFlags
 */
enum class ResultAbnormalFlags
{
   None,
   BelowLowNormal,        // L
   AboveHighNormal,       // H
   BelowPanicNormal,      // LL
   AbovePanicHigh,        // HH
   BelowAbsolutelow,      // <
   AboveAbsoluteHigh,     // >
   Normal,                // N
   Abnormal,              // A
   SignificantChangeUp,   // U
   SignificantChangeDown, // D
   Better,                // B
   Worse                  // W
};

/** \ingroup LIS
 * ResultNatureOfAbnormalityTestingSet
 */
enum class ResultNatureOfAbnormalityTestingSet
{
   None   = '0',
   Age    = 'A',
   Sex    = 'S',
   Race   = 'R',
   Normal = 'N'
};

/** \ingroup LIS
 * ResultStatus
 */
enum class ResultStatus
{
   None                       = '0',
   Correction                 = 'C',
   PreliminaryResults         = 'P',
   FinalResults               = 'F',
   CannotBeDone               = 'X',
   ResultsPending             = 'I',
   PartialResults             = 'S',
   MICLevel                   = 'M',
   PreviouslyTransmitted      = 'R',
   NecessaryInformation       = 'N',
   ResponseToOutstandingQuery = 'Q',
   ApprovedResult             = 'V',
   Warning                    = 'W'
};

/** \ingroup LIS
 * ResultRecord
 * CLSI LIS02-A2 Result Record
 */
class ResultRecord
   : public Record
{
public:
   ResultRecord(const std::string& lisString = "", int fields = 14);
   ~ResultRecord() = default;

   std::string recordTypeID() { return getFieldValue(1); }                 // 1
   int sequenceNumber()       { return std::stoi(getFieldValue(2)); }      // 2
   UniversalTestID testID()   { return _testID; }                          // 3
   std::string data()         { return getFieldValue(4); }                 // 4
   std::string units()        { return getFieldValue(5); }                 // 5

   std::string dilutionFactor() { return getFieldValue(6); }               // 6     DxH500 Only
   std::string referenceRanges();                                          // 6     // 7
   std::string abnormalFlag();                                             // 7     // 8
   ResultAbnormalFlags abnormalFlagEnum();                                 // 7     // 8
   std::string natureOfAbnormalityTesting();                               // 8     // 9
   ResultNatureOfAbnormalityTestingSet natureOfAbnormalityTestingEnum();   // 8     // 9
   std::string status();                                                   // 9     // 10
   ResultStatus statusEnum();                                              // 9     // 10
   std::string dateChangeValuesOrUnits();                                  // 10    // 11
   std::string operatorIdentification();                                   // 11    // 12
   std::string dateTimeStarted();                                          // 12    // 13
   std::string dateTimeCompleted();                                        // 13    // 14
   std::string instrumentID();                                             // 14    // 15

   void setRecordTypeID(const char id) { setFieldValue(1, std::string(1, id)); }    // 1
   void setSequenceNumber(int number) { setFieldValue(2, std::to_string(number)); } // 2
   void setTestID(UniversalTestID testID);                                          // 3
   void setData(const std::string& val) { setFieldValue(4, val); }                  // 4
   void setUnits(const std::string& val) { setFieldValue(5, val); }                 // 5
   void setDilutionFactor(const std::string& val) { setFieldValue(6, val); }        // 6     DxH500 Only
   void setReferenceRanges(const std::string& val);                                 // 6     // 7
   void setAbnormalFlag(const std::string& val);                                    // 7     // 8
   void setAbnormalFlag(ResultAbnormalFlags flag);                                  // 7     // 8
   void setNatureOfAbnormalityTesting(const std::string& val);                      // 8     // 9
   void setNatureOfAbnormalityTesting(ResultNatureOfAbnormalityTestingSet value);   // 8     // 9
   void setStatus(const std::string& val);                                          // 9     // 10
   void setStatus(ResultStatus status);                                             // 9     // 10
   void setDateChangeValuesOrUnits(const std::string& val);                         // 10    // 11
   void setOperatorIdentification(const std::string& val);                          // 11    // 12
   void setDateTimeStarted(const std::string& val);                                 // 12    // 13
   void setDateTimeCompleted(const std::string& val);                               // 13    // 14
   void setInstrumentID(const std::string& val);                                    // 14    // 15

private:
   UniversalTestID _testID;

protected:
   virtual void initFields();
};

} // namespace lis2a
} // namespace tbs 