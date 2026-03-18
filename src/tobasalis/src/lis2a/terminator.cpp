#include "tobasalis/lis2a/terminator.h"

namespace tbs {
namespace lis2a {

TerminatorRecord::TerminatorRecord(const std::string& lisString)
   : Record(lisString)
{
   setRecordType(RecordType::Terminator);
   _totalField = 3;
   initFields();
}

void TerminatorRecord::initFields()
{
   _lisFields.push_back(Record::createField(1, "RecordTypeID", "L"));
   _lisFields.push_back(Record::createField(2, "SequenceNumber", "1"));
   _lisFields.push_back(Record::createField(3, "TerminationCode", "N"));
}

TerminationCode TerminatorRecord::codeEnum()
{
   std::string value = code();

   if (value == "N")
      return TerminationCode::Normal;
   if (value == "T")
      return TerminationCode::SenderAborted;
   if (value == "R")
      return TerminationCode::ReceiverRequestedAbort;
   if (value == "E")
      return TerminationCode::UnknownSystemError;
   if (value == "Q")
      return TerminationCode::ErrorInLastRequestForInformation;
   if (value == "I")
      return TerminationCode::NoInformationAvailableFromLastQuery;
   if (value == "F")
      return TerminationCode::LastRequestForInformationProcessed;

   return TerminationCode::Normal;
}

void TerminatorRecord::setCode(TerminationCode code)
{
   switch (code)
   {
   case TerminationCode::Normal:
      setCode("N");
      break;
   case TerminationCode::SenderAborted:
      setCode("T");
      break;
   case TerminationCode::ReceiverRequestedAbort:
      setCode("R");
      break;
   case TerminationCode::UnknownSystemError:
      setCode("E");
      break;
   case TerminationCode::ErrorInLastRequestForInformation:
      setCode("Q");
      break;
   case TerminationCode::NoInformationAvailableFromLastQuery:
      setCode("I");
      break;
   case TerminationCode::LastRequestForInformationProcessed:
      setCode("F");
      break;
   }
}

} // namespace lis2a
} // namespace tbs