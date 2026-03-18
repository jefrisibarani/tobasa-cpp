#pragma once

#include "tobasalis/lis2a/substartingrange.h"
#include "tobasalis/lis2a/subuniversaltest.h"

namespace tbs {
namespace lis2a {

/** \ingroup LIS
 * QueryStatusCode
 */
enum class QueryStatusCode
{
   Correction                = 'C',
   Preliminary               = 'P',
   Final                     = 'F',
   Canceled                  = 'X',
   Pending                   = 'I',
   Unfinalized               = 'S',
   MICLevel                  = 'M',
   PreviouslyTransmitted     = 'R',
   CancelLastRequestCriteria = 'A',
   RequestingNewOrEdited     = 'N',
   RequestingTestOrder       = 'O',
   RequestDemographics       = 'D',
   None
};

/** \ingroup LIS
 * RequestInfoRecord
 * CLSI LIS02-A2 (formerly ASTM 1394-97) Request Information Record
 */
class RequestInfoRecord
   : public Record
{
public:
   RequestInfoRecord(const std::string& lisString = "");
   ~RequestInfoRecord() = default;

   std::string recordTypeID()                { return getFieldValue(1); }                          // 1
   int sequenceNumber()                      { return std::stoi(getFieldValue(2)); }               // 2
   StartingRange& startRange()               { return _startRange; }                               // 3
   StartingRange& endRange()                 { return _endRange; }                                 // 4
   UniversalTestID& testID()                 { return _testID; }                                   // 5
   std::string requestTimeLimit()            { return getFieldValue(6); }                          // 6
   std::string beginRequestResultsDateTime() { return getFieldValue(7); }                          // 7
   std::string endRequestResultsDateTime()   { return getFieldValue(8); }                          // 8
   std::string requestingPhysicianName()     { return getFieldValue(9); }                          // 9
   std::string requestingPhysicianPhone()    { return getFieldValue(10); }                         // 10
   std::string userField1()                  { return getFieldValue(11); }                         // 11
   std::string userField2()                  { return getFieldValue(12); }                         // 12
   std::string requestInfoStatusCode()       { return getFieldValue(13); }                         // 13
   QueryStatusCode requestInfoStatusCodeEnum();

   void setRecordTypeID(const char id)             { setFieldValue(1, std::string(1, id)); }       // 1
   void setSequenceNumber(int number)              { setFieldValue(2, std::to_string(number)); }   // 2
   void setStartingRange(StartingRange start);                                                     // 3
   void setEndingRange(StartingRange end);                                                         // 4
   void setTestID(UniversalTestID testID);                                                         // 5
   void setRequestTimeLimit(const std::string& val)            { return setFieldValue(6, val);}    // 6
   void setBeginRequestResultsDateTime(const std::string& val) { return setFieldValue(7, val); }   // 7
   void setEndRequestResultsDateTime(const std::string& val)   { return setFieldValue(8, val); }   // 8
   void setRequestingPhysicianName(const std::string& val)     { return setFieldValue(9, val); }   // 9
   void setRequestingPhysicianPhone(const std::string& val)    { return setFieldValue(10, val); }  // 10
   void setUserField1(const std::string& val)                  { return setFieldValue(11, val); }  // 11
   void setUserField2(const std::string& val)                  { return setFieldValue(12, val); }  // 12
   void setRequestInfoStatusCode(const std::string& val)       { return setFieldValue(13, val); }  // 13
   void setRequestInfoStatusCode(QueryStatusCode code);                                            // 13

private:
   StartingRange     _startRange;
   StartingRange     _endRange;
   UniversalTestID   _testID;

protected:
   virtual void initFields();
};

} // namespace lis2a
} // namespace tbs 