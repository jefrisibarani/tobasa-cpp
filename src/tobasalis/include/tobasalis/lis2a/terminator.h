#pragma once

#include "tobasalis/lis2a/record.h"

namespace tbs {
namespace lis2a {

/** \ingroup LIS
 * TerminationCode
 */
enum class TerminationCode
{
   Normal                              = 'N',
   SenderAborted                       = 'T',
   ReceiverRequestedAbort              = 'R',
   UnknownSystemError                  = 'E',
   ErrorInLastRequestForInformation    = 'Q',
   NoInformationAvailableFromLastQuery = 'I',
   LastRequestForInformationProcessed  = 'F'
};

/** TerminatorRecord
 * CLSI LIS2-A Message Terminator Record
 */
class TerminatorRecord
   : public Record
{
public:
   TerminatorRecord(const std::string& lisString = "");
   ~TerminatorRecord() = default;

   std::string recordTypeID()    { return getFieldValue(1); }
   int         sequenceNumber()  { return std::stoi(getFieldValue(2)); }
   std::string code()            { return getFieldValue(3); }
   TerminationCode codeEnum();

   void setRecordTypeID(const std::string& val) { setFieldValue(1, val); }
   void setSequenceNumber(int number)           { setFieldValue(2, std::to_string(number)); }
   void setCode(const std::string& code)        { setFieldValue(3, code); }
   void setCode(TerminationCode code);

protected:
   virtual void initFields();
};

} // namespace lis2a
} // namespace tbs 