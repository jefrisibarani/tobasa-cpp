#include <string>
#include <tobasa/logger.h>
#include "tobasalis/lis2a/requestinfo.h"

namespace tbs {
namespace lis2a {

RequestInfoRecord::RequestInfoRecord(const std::string& lisString)
   : Record(lisString)
{
   setRecordType(RecordType::Query);
   _totalField = 13;
   initFields();
}

void RequestInfoRecord::initFields()
{
   _lisFields.push_back(Record::createField(1, "RecordTypeID", "Q"));
   _lisFields.push_back(Record::createField(2, "SequenceNumber"));

   LisField field3(3, "StartRange");
   field3.setValueType(FieldValueType::SubRecord);
   field3.setType(FieldType::Component);
   field3.setSubRecord(&_startRange);
   _lisFields.push_back(field3);

   LisField field4(4, "EndRange");
   field4.setValueType(FieldValueType::SubRecord);
   field4.setType(FieldType::Component);
   field4.setSubRecord(&_endRange);
   _lisFields.push_back(field4);

   LisField field5(5, "TestID");
   field5.setValueType(FieldValueType::SubRecord);
   field5.setType(FieldType::Component);
   field5.setSubRecord(&_testID);
   _lisFields.push_back(field5);

   _lisFields.push_back(Record::createField(6, "RequestTimeLimit"));
   _lisFields.push_back(Record::createField(7, "BeginRequestResultsDateTime"));
   _lisFields.push_back(Record::createField(8, "EndRequestResultsDateTime"));
   _lisFields.push_back(Record::createField(9, "RequestingPhysicianName"));
   _lisFields.push_back(Record::createField(10, "RequestingPhysicianPhone"));
   _lisFields.push_back(Record::createField(11, "UserField1"));
   _lisFields.push_back(Record::createField(12, "UserField2"));
   _lisFields.push_back(Record::createField(13, "RequestInfoStatusCode"));
}

QueryStatusCode RequestInfoRecord::requestInfoStatusCodeEnum()
{
   std::string value = getFieldValue(13);

   if (value == "C")
      return QueryStatusCode::Correction;
   if (value == "P")
      return QueryStatusCode::Preliminary;
   if (value == "F")
      return QueryStatusCode::Final;
   if (value == "X")
      return QueryStatusCode::Canceled;
   if (value == "I")
      return QueryStatusCode::Pending;
   if (value == "S")
      return QueryStatusCode::Unfinalized;
   if (value == "M")
      return QueryStatusCode::MICLevel;
   if (value == "R")
      return QueryStatusCode::PreviouslyTransmitted;
   if (value == "A")
      return QueryStatusCode::CancelLastRequestCriteria;
   if (value == "N")
      return QueryStatusCode::RequestingNewOrEdited;
   if (value == "O")
      return QueryStatusCode::RequestingTestOrder;
   if (value == "D")
      return QueryStatusCode::RequestDemographics;

   return QueryStatusCode::None;
}


void RequestInfoRecord::setStartingRange(StartingRange start)
{
   _startRange.fromString(start.toString());
}

void RequestInfoRecord::setEndingRange(StartingRange end)
{
   _endRange.fromString(end.toString());
}

void RequestInfoRecord::setTestID(UniversalTestID testID)
{
   _testID.fromString(testID.toString());
}

void RequestInfoRecord::setRequestInfoStatusCode(QueryStatusCode code)
{
   switch (code)
   {
   case QueryStatusCode::Correction:
      setFieldValue(13, "C");
      break;
   case QueryStatusCode::Preliminary:
      setFieldValue(13, "P");
      break;
   case QueryStatusCode::Final:
      setFieldValue(13, "F");
      break;
   case QueryStatusCode::Canceled:
      setFieldValue(13, "X");
      break;
   case QueryStatusCode::Pending:
      setFieldValue(13, "I");
      break;
   case QueryStatusCode::Unfinalized:
      setFieldValue(13, "S");
      break;
   case QueryStatusCode::MICLevel:
      setFieldValue(13, "M");
      break;
   case QueryStatusCode::PreviouslyTransmitted:
      setFieldValue(13, "R");
      break;
   case QueryStatusCode::CancelLastRequestCriteria:
      setFieldValue(13, "A");
      break;
   case QueryStatusCode::RequestingNewOrEdited:
      setFieldValue(13, "N");
      break;
   case QueryStatusCode::RequestingTestOrder:
      setFieldValue(13, "O");
      break;
   case QueryStatusCode::RequestDemographics:
      setFieldValue(13, "D");
      break;
   }
}

} // namespace lis2a
} // namespace tbs