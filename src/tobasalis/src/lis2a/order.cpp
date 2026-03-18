#include <string>
#include "tobasalis/lis2a/order.h"

namespace tbs {
namespace lis2a {

OrderRecord::OrderRecord(const std::string& lisString)
   : Record(lisString)
{
   setRecordType(RecordType::Order);
   _totalField = 31;
   initFields();
}

void OrderRecord::initFields()
{
   _lisFields.push_back(Record::createField(1, "RecordTypeID", "O"));
   _lisFields.push_back(Record::createField(2, "SequenceNumber"));
   _lisFields.push_back(Record::createField(3, "SpecimenID"));
   _lisFields.push_back(Record::createField(4, "AnalyzerSpecimenID"));

   LisField field5(5, "TestID");
   field5.setValueType(FieldValueType::SubRecord);
   field5.setType(FieldType::Component);
   field5.setSubRecord(&_univSubRecord);
   _lisFields.push_back(field5);

   _lisFields.push_back(Record::createField(6, "Priority"));
   _lisFields.push_back(Record::createField(7, "OrderedDateTime"));
   _lisFields.push_back(Record::createField(8, "SpecimenCollectionDateTime"));
   _lisFields.push_back(Record::createField(9, "CollectionEndTime"));
   _lisFields.push_back(Record::createField(10, "CollectionVolume"));
   _lisFields.push_back(Record::createField(11, "CollectorID"));
   _lisFields.push_back(Record::createField(12, "ActionCode"));
   _lisFields.push_back(Record::createField(13, "DangerCode"));
   _lisFields.push_back(Record::createField(14, "RelevantClinicalInfo"));
   _lisFields.push_back(Record::createField(15, "DateTimeSpecimenReceived"));
   _lisFields.push_back(Record::createField(16, "SpecimenDescriptor"));
   _lisFields.push_back(Record::createField(17, "OrderingPhysician"));
   _lisFields.push_back(Record::createField(18, "PhysicianPoneNumber"));
   _lisFields.push_back(Record::createField(19, "UserField1"));
   _lisFields.push_back(Record::createField(20, "UserField1"));
   _lisFields.push_back(Record::createField(21, "LaboratoryField1"));
   _lisFields.push_back(Record::createField(22, "LaboratoryField2"));
   _lisFields.push_back(Record::createField(23, "LastModified"));
   _lisFields.push_back(Record::createField(24, "AnalyzerChargetoIS"));
   _lisFields.push_back(Record::createField(25, "InstrumentSectionID"));
   _lisFields.push_back(Record::createField(26, "ReportType"));
   _lisFields.push_back(Record::createField(27, "ReservedFiedld"));
   _lisFields.push_back(Record::createField(28, "LocationOfSpecimenColl"));
   _lisFields.push_back(Record::createField(29, "NosocomialInfectionFlag"));
   _lisFields.push_back(Record::createField(30, "SpecimenService"));
   _lisFields.push_back(Record::createField(31, "SpecimenInstitution"));
}

UniversalTestID& OrderRecord::testID()
{
   return _univSubRecord;
}

OrderPriority OrderRecord::priorityEnum()
{
   std::string value = getFieldValue(6);

   if (value == "S")
      return OrderPriority::Stat;
   if (value == "A")
      return OrderPriority::ASAP;
   if (value == "R")
      return OrderPriority::Routine;
   if (value == "C")
      return OrderPriority::CallBack;
   if (value == "P")
      return OrderPriority::PreOperative;

   return OrderPriority::None;
}

OrderActionCode OrderRecord::actionCodeEnum()
{
   std::string value = getFieldValue(12);

   if (value == "C")
      return OrderActionCode::Cancel;
   if (value == "A")
      return OrderActionCode::Add;
   if (value == "N")
      return OrderActionCode::New;
   if (value == "P")
      return OrderActionCode::Pending;
   if (value == "L")
      return OrderActionCode::Reserved;
   if (value == "X")
      return OrderActionCode::InProcess;
   if (value == "Q")
      return OrderActionCode::QC;
   if (value == "D")
      return OrderActionCode::Pooled;      

   return OrderActionCode::None;
}

OrderReportType OrderRecord::reportTypeEnum()
{
   std::string value = getFieldValue(26);

   if (value == "O")
      return OrderReportType::Order;
   if (value == "C")
      return OrderReportType::Correction;
   if (value == "P")
      return OrderReportType::Preliminary;
   if (value == "F")
      return OrderReportType::Final;
   if (value == "X")
      return OrderReportType::Cancelled;
   if (value == "I")
      return OrderReportType::Pending;
   if (value == "Y")
      return OrderReportType::NoOrder;
   if (value == "Z")
      return OrderReportType::NoRecord;
   if (value == "Q")
      return OrderReportType::Response;

   return OrderReportType::None;
}


void OrderRecord::setTestID(UniversalTestID testID)
{
   auto val = testID.toString();
   _univSubRecord.fromString(val);
}

void OrderRecord::setPriority(OrderPriority priority)
{
   switch (priority)
   {
   case OrderPriority::Stat:
      setFieldValue(6, "S");
      break;
   case OrderPriority::ASAP:
      setFieldValue(6, "A");
      break;
   case OrderPriority::Routine:
      setFieldValue(6, "R");
      break;
   case OrderPriority::CallBack:
      setFieldValue(6, "C");
      break;
   case OrderPriority::PreOperative:
      setFieldValue(6, "P");
      break;
   case OrderPriority::None:
      break;
   }
}

void OrderRecord::setActionCode(OrderActionCode code)
{
   switch (code)
   {
   case OrderActionCode::Cancel:
      setFieldValue(12, "C");
      break;
   case OrderActionCode::Add:
      setFieldValue(12, "A");
      break;
   case OrderActionCode::New:
      setFieldValue(12, "N");
      break;
   case OrderActionCode::Pending:
      setFieldValue(12, "P");
      break;
   case OrderActionCode::Reserved:
      setFieldValue(12, "L");
      break;
   case OrderActionCode::InProcess:
      setFieldValue(12, "X");
      break;
   case OrderActionCode::QC:
      setFieldValue(12, "Q");
      break;
   case OrderActionCode::None:
      break;
   }
}

void OrderRecord::setReportType(OrderReportType type)
{
   switch (type)
   {
   case OrderReportType::Order:
      setFieldValue(26, "O");
      break;
   case OrderReportType::Correction:
      setFieldValue(26, "C");
      break;
   case OrderReportType::Preliminary:
      setFieldValue(26, "P");
      break;
   case OrderReportType::Final:
      setFieldValue(26, "F");
      break;
   case OrderReportType::Cancelled:
      setFieldValue(26, "X");
      break;
   case OrderReportType::Pending:
      setFieldValue(26, "I");
      break;
   case OrderReportType::NoOrder:
      setFieldValue(26, "Y");
      break;
   case OrderReportType::NoRecord:
      setFieldValue(26, "Z");
      break;
   case OrderReportType::Response:
      setFieldValue(26, "Q");
      break;
   }
}

int OrderRecord::sequenceNumber() 
{ 
   auto value = getFieldValue(2);
   return std::stoi(value); 
}

} // namespace lis2a
} // namespace tbs