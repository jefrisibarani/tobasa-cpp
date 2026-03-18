#include <string>
#include "tobasa/datetime.h"
#include <tobasa/logger.h>
#include "tobasalis/lis2a/header.h"

namespace tbs {
namespace lis2a {

HeaderRecord::HeaderRecord(const std::string& lisString, lis::Delimiter delimiter )
   : Record(lisString)
{
   setRecordType(RecordType::Header);
   _totalField = 14;

   if ( delimiter.initialized() )
      _delimiter = delimiter;

   if (!_lisString.empty() )
   {
      if ( !delimiter.initialized() )
      {
         // use delimiter from raw header text
         // H|\\^&|||1^LIS host^1.0|||||||P|LIS2A|20130129102030
         fieldDelimiter    ( static_cast<char>(lisString[1]) );
         repeatDelimiter   ( static_cast<char>(lisString[2]) );
         componentDelimiter( static_cast<char>(lisString[3]) );
         escapeCharacter   ( static_cast<char>(lisString[4]) );
      }
      else
         _delimiter = delimiter;
   }

   tbs::Logger::logD("[lis_record] Initializing HeaderRecord: fieldDelimiter: {} , repeatDelimiter: {} , componentDelimiter: {} , escapeCharacter: {}",
      fieldDelimiter(), repeatDelimiter(), componentDelimiter(), escapeCharacter() );

   initFields();
}

void HeaderRecord::initFields()
{
   _lisFields.push_back(Record::createField(1, "RecordTypeID", std::string(1, 'H')));

   std::string delimiterStr;
   delimiterStr.append(1, repeatDelimiter() );
   delimiterStr.append(1, componentDelimiter() );
   delimiterStr.append(1, escapeCharacter() );

   _lisFields.push_back(Record::createField(2, "Delimiter", delimiterStr));
   _lisFields.push_back(Record::createField(3, "MessageControlID"));
   _lisFields.push_back(Record::createField(4, "AccessPassword"));
   _lisFields.push_back(Record::createField(5, "SenderID"));
   _lisFields.push_back(Record::createField(6, "SenderStreetAddress"));
   _lisFields.push_back(Record::createField(7, "Reserved"));
   _lisFields.push_back(Record::createField(8, "SenderTelephoneNumber"));
   _lisFields.push_back(Record::createField(9, "CharacteristicsOfSender"));
   _lisFields.push_back(Record::createField(10, "ReceiverID"));
   _lisFields.push_back(Record::createField(11, "Comment"));

   LisField info12(12, "ProcessingID");
   info12.setValueType(FieldValueType::Enum);
   _lisFields.push_back(info12);

   _lisFields.push_back(Record::createField(13, "Version", "LIS2-A2"));

   LisField info14(14, "MessageDateTime", tbs::DateTime::now().format("{:%Y%m%d%H%M%S}"));
   info14.setValueType(FieldValueType::DateTime);
   _lisFields.push_back(info14);
}

HeaderProcessingID HeaderRecord::processingIDEnum()
{
   std::string procIdStr = getFieldValue(12);

   if (procIdStr == "P")
      return HeaderProcessingID::Production;
   else if (procIdStr == "T")
      return HeaderProcessingID::Training;
   else if (procIdStr == "D")
      return HeaderProcessingID::Debugging;
   else if (procIdStr == "Q")
      return HeaderProcessingID::QualityControl;
   else
      return HeaderProcessingID::Unknown;
}

void HeaderRecord::setProcessingID(HeaderProcessingID procId)
{
   switch (procId)
   {
   case HeaderProcessingID::Production:
      setFieldValue(12, "P");
      break;
   case HeaderProcessingID::Training:
      setFieldValue(12, "T");
      break;
   case HeaderProcessingID::Debugging:
      setFieldValue(12, "D");
      break;
   case HeaderProcessingID::QualityControl:
      setFieldValue(12, "Q");
      break;
   }
}

} // namespace lis2a
} // namespace tbs
