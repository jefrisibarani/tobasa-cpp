#include <string>
#include "tobasalis/lis2a/result.h"

namespace tbs {
namespace lis2a {

ResultRecord::ResultRecord(const std::string& lisString, int fields)
   : Record(lisString)
{
   setRecordType(RecordType::Result);
   _totalField = fields;
   initFields();
}

void ResultRecord::initFields()
{
   _lisFields.push_back(Record::createField(1, "RecordTypeID", "R"));
   _lisFields.push_back(Record::createField(2, "SequenceNumber"));

   LisField field3(3, "TestID");
   field3.setValueType(FieldValueType::SubRecord);
   field3.setType(FieldType::Component);
   field3.setSubRecord(&_testID);
   _lisFields.push_back(field3);

   _lisFields.push_back(Record::createField(4, "Data"));
   _lisFields.push_back(Record::createField(5, "Units"));

   if (_totalField == 14)
   {
      _lisFields.push_back(Record::createField(6, "ReferenceRanges"));
      _lisFields.push_back(Record::createField(7, "AbnormalFlag"));
      _lisFields.push_back(Record::createField(8, "NatureOfAbnormalityTesting"));
      _lisFields.push_back(Record::createField(9, "Status"));
      _lisFields.push_back(Record::createField(10, "DateChangeValuesOrUnits"));
      _lisFields.push_back(Record::createField(11, "OperatorIdentification"));
      _lisFields.push_back(Record::createField(12, "DateTimeStarted"));
      _lisFields.push_back(Record::createField(13, "DateTimeCompleted"));
      _lisFields.push_back(Record::createField(14, "InstrumentID"));
   }
   else if (_totalField == 15)
   {
      // DxH500 has 15 fields for Result Record (extra DilutionFactor)
      _lisFields.push_back(Record::createField(6, "DilutionFactor"));
      _lisFields.push_back(Record::createField(7, "ReferenceRanges"));
      _lisFields.push_back(Record::createField(8, "AbnormalFlag"));
      _lisFields.push_back(Record::createField(9, "NatureOfAbnormalityTesting"));
      _lisFields.push_back(Record::createField(10, "Status"));
      _lisFields.push_back(Record::createField(11, "DateChangeValuesOrUnits"));
      _lisFields.push_back(Record::createField(12, "OperatorIdentification"));
      _lisFields.push_back(Record::createField(13, "DateTimeStarted"));
      _lisFields.push_back(Record::createField(14, "DateTimeCompleted"));
      _lisFields.push_back(Record::createField(15, "InstrumentID"));
   }
}

std::string ResultRecord::referenceRanges()              // 6  // 7
{
   if (_totalField == 15)
      return getFieldValue(7);
   else
      return getFieldValue(6);
}

std::string ResultRecord::abnormalFlag()
{
   if (_totalField == 15)
      return getFieldValue(8);
   else
      return getFieldValue(7);
}

ResultAbnormalFlags ResultRecord::abnormalFlagEnum()     // 7  // 8
{
   std::string value = abnormalFlag();

   if (value == "L")
      return ResultAbnormalFlags::BelowLowNormal;
   if (value == "H")
      return ResultAbnormalFlags::AboveHighNormal;
   if (value == "LL")
      return ResultAbnormalFlags::BelowPanicNormal;
   if (value == "HH")
      return ResultAbnormalFlags::AbovePanicHigh;
   if (value == "<")
      return ResultAbnormalFlags::BelowAbsolutelow;
   if (value == ">")
      return ResultAbnormalFlags::AboveAbsoluteHigh;
   if (value == "N")
      return ResultAbnormalFlags::Normal;
   if (value == "A")
      return ResultAbnormalFlags::Abnormal;
   if (value == "U")
      return ResultAbnormalFlags::SignificantChangeUp;
   if (value == "D")
      return ResultAbnormalFlags::SignificantChangeDown;
   if (value == "B")
      return ResultAbnormalFlags::Better;
   if (value == "W")
      return ResultAbnormalFlags::Worse;

   return ResultAbnormalFlags::None;
}

std::string ResultRecord::natureOfAbnormalityTesting()   // 8  // 9
{
   if (_totalField == 15)
      return getFieldValue(9);
   else
      return getFieldValue(8);
}

ResultNatureOfAbnormalityTestingSet ResultRecord::natureOfAbnormalityTestingEnum() // 8   // 9
{
   std::string value = natureOfAbnormalityTesting();

   if (value == "A")
      return ResultNatureOfAbnormalityTestingSet::Age;
   if (value == "S")
      return ResultNatureOfAbnormalityTestingSet::Sex;
   if (value == "R")
      return ResultNatureOfAbnormalityTestingSet::Race;
   if (value == "N")
      return ResultNatureOfAbnormalityTestingSet::Normal;

   return ResultNatureOfAbnormalityTestingSet::None;
}

std::string ResultRecord::status()                     // 9 // 10
{
   if (_totalField == 15)
      return getFieldValue(10);
   else
      return getFieldValue(9);
}

ResultStatus ResultRecord::statusEnum()                // 9 // 10
{
   std::string value = status();

   if (value == "C")
      return ResultStatus::Correction;
   if (value == "P")
      return ResultStatus::PreliminaryResults;
   if (value == "F")
      return ResultStatus::FinalResults;
   if (value == "X")
      return ResultStatus::CannotBeDone;
   if (value == "I")
      return ResultStatus::ResultsPending;
   if (value == "S")
      return ResultStatus::PartialResults;
   if (value == "M")
      return ResultStatus::MICLevel;
   if (value == "R")
      return ResultStatus::PreviouslyTransmitted;
   if (value == "N")
      return ResultStatus::NecessaryInformation;
   if (value == "Q")
      return ResultStatus::ResponseToOutstandingQuery;
   if (value == "V")
      return ResultStatus::ApprovedResult;
   if (value == "W")
      return ResultStatus::Warning;
   if (value == "Null")
      return ResultStatus::None;

   return ResultStatus::None;
}

std::string ResultRecord::dateChangeValuesOrUnits()      // 10 // 11
{
   if (_totalField == 15)
      return getFieldValue(11);
   else
      return getFieldValue(10);
}

std::string ResultRecord::operatorIdentification()       // 11 // 12
{
   if (_totalField == 15)
      return getFieldValue(12);
   else
      return getFieldValue(11);
}

std::string ResultRecord::dateTimeStarted()              // 12 // 13
{
   if (_totalField == 15)
      return getFieldValue(13);
   else
      return getFieldValue(12);
}

std::string ResultRecord::dateTimeCompleted()            // 13 // 14
{
   if (_totalField == 15)
      return getFieldValue(14);
   else
      return getFieldValue(13);
}

std::string ResultRecord::instrumentID()                 // 14 // 15
{
   if (_totalField == 15)
      return getFieldValue(15);
   else
      return getFieldValue(14);
}

void ResultRecord::setTestID(UniversalTestID testID)              // 3
{
   _testID.fromString(testID.toString());
}

void ResultRecord::setReferenceRanges(const std::string& val)      // 6   // 7
{
   if (_totalField == 15)
      return setFieldValue(7, val);
   else
      return setFieldValue(6, val);
}

void ResultRecord::setAbnormalFlag(const std::string& val)        // 7   // 8
{
   if (_totalField == 15)
      return setFieldValue(8, val);
   else
      return setFieldValue(7, val);
}

void ResultRecord::setAbnormalFlag(ResultAbnormalFlags flag)      // 7  // 8
{
   switch (flag)
   {
   case ResultAbnormalFlags::BelowLowNormal:
      setAbnormalFlag("L");
      break;
   case ResultAbnormalFlags::AboveHighNormal:
      setAbnormalFlag("H");
      break;
   case ResultAbnormalFlags::BelowPanicNormal:
      setAbnormalFlag("LL");
      break;
   case ResultAbnormalFlags::AbovePanicHigh:
      setAbnormalFlag("HH");
      break;
   case ResultAbnormalFlags::BelowAbsolutelow:
      setAbnormalFlag("<");
      break;
   case ResultAbnormalFlags::AboveAbsoluteHigh:
      setAbnormalFlag(">");
      break;
   case ResultAbnormalFlags::Normal:
      setAbnormalFlag("N");
      break;
   case ResultAbnormalFlags::Abnormal:
      setAbnormalFlag("A");
      break;
   case ResultAbnormalFlags::SignificantChangeUp:
      setAbnormalFlag("U");
      break;
   case ResultAbnormalFlags::SignificantChangeDown:
      setAbnormalFlag("D");
      break;
   case ResultAbnormalFlags::Better:
      setAbnormalFlag("B");
      break;
   case ResultAbnormalFlags::Worse:
      setAbnormalFlag("W");
      break;
   case ResultAbnormalFlags::None:
      setAbnormalFlag("0");
      break;
   }
}

void ResultRecord::setNatureOfAbnormalityTesting(const std::string& val)   // 8  // 9
{
   if (_totalField == 15)
      return setFieldValue(9, val);
   else
      return setFieldValue(8, val);
}

void ResultRecord::setNatureOfAbnormalityTesting(ResultNatureOfAbnormalityTestingSet value)  // 8  // 9
{
   switch (value)
   {
   case ResultNatureOfAbnormalityTestingSet::Age:
      setNatureOfAbnormalityTesting("A");
      break;
   case ResultNatureOfAbnormalityTestingSet::Sex:
      setNatureOfAbnormalityTesting("S");
      break;
   case ResultNatureOfAbnormalityTestingSet::Race:
      setNatureOfAbnormalityTesting("R");
      break;
   case ResultNatureOfAbnormalityTestingSet::Normal:
      setNatureOfAbnormalityTesting("N");
      break;
   case ResultNatureOfAbnormalityTestingSet::None:
      setNatureOfAbnormalityTesting("0");
      break;
   }
}

void ResultRecord::setStatus(const std::string& val)        // 9     // 10
{
   if (_totalField == 15)
      return setFieldValue(10, val);
   else
      return setFieldValue(9, val);
}

void ResultRecord::setStatus(ResultStatus status)           // 9     // 10
{
   switch (status)
   {
   case ResultStatus::Correction:
      setStatus("C");
      break;
   case ResultStatus::PreliminaryResults:
      setStatus("P");
      break;
   case ResultStatus::FinalResults:
      setStatus("F");
      break;
   case ResultStatus::CannotBeDone:
      setStatus("X");
      break;
   case ResultStatus::ResultsPending:
      setStatus("I");
      break;
   case ResultStatus::PartialResults:
      setStatus("S");
      break;
   case ResultStatus::MICLevel:
      setStatus("M");
      break;
   case ResultStatus::PreviouslyTransmitted:
      setStatus("R");
      break;
   case ResultStatus::NecessaryInformation:
      setStatus("N");
      break;
   case ResultStatus::ResponseToOutstandingQuery:
      setStatus("Q");
      break;
   case ResultStatus::ApprovedResult:
      setStatus("V");
      break;
   case ResultStatus::Warning:
      setStatus("W");
      break;
   case ResultStatus::None:
      setStatus("Null");
      break;
   }
}

void ResultRecord::setDateChangeValuesOrUnits(const std::string& val)         // 10 // 11
{
   if (_totalField == 15)
      return setFieldValue(11, val);
   else
      return setFieldValue(10, val);
}

void ResultRecord::setOperatorIdentification(const std::string& val)          // 11  // 12
{
   if (_totalField == 15)
      return setFieldValue(12, val);
   else
      return setFieldValue(11, val);
}

void ResultRecord::setDateTimeStarted(const std::string& val)                  // 12 // 13
{
   if (_totalField == 15)
      return setFieldValue(13, val);
   else
      return setFieldValue(12, val);
}

void ResultRecord::setDateTimeCompleted(const std::string& val)                // 13  // 14
{
   if (_totalField == 15)
      return setFieldValue(14, val);
   else
      return setFieldValue(13, val);
}

void ResultRecord::setInstrumentID(const std::string& val)                     // 14   // 15
{
   if (_totalField == 15)
      return setFieldValue(15, val);
   else
      return setFieldValue(14, val);
}

} // namespace lis2a
} // namespace tbs